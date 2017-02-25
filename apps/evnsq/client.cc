#include "client.h"

#include <rapidjson/document.h>
#include <evpp/event_loop.h>
#include <evpp/tcp_client.h>
#include <evpp/tcp_conn.h>
#include <evpp/httpc/request.h>
#include <evpp/httpc/response.h>
#include <evpp/httpc/conn.h>

#include "command.h"
#include "option.h"

namespace evnsq {
static const std::string kNSQMagic = "  V2";
static const std::string kOK = "OK";

Client::Client(evpp::EventLoop* l, Type t, const Option& ops)
    : loop_(l), type_(t), option_(ops) {}

Client::~Client() {}

void Client::ConnectToNSQD(const std::string& addr) {
    auto c = ConnPtr(new NSQConn(this, option_));
    connecting_conns_[addr] = c;
    c->SetMessageCallback(msg_fn_);
    c->SetConnectionCallback(std::bind(&Client::OnConnection, this, std::placeholders::_1));
    c->Connect(addr);
}

void Client::ConnectToNSQDs(const std::string& addrs/*host1:port1,host2:port2*/) {
    std::vector<std::string> v;
    evpp::StringSplit(addrs, ",", 0, v);
    ConnectToNSQDs(v);
}

void Client::ConnectToNSQDs(const std::vector<std::string>& tcp_addrs/*host:port*/) {
    for (auto it = tcp_addrs.begin(); it != tcp_addrs.end(); ++it) {
        ConnectToNSQD(*it);
    }
}

void Client::ConnectToLoopupd(const std::string& lookupd_url/*http://127.0.0.1:4161/lookup?topic=test*/) {
    auto f = [this, lookupd_url]() {
        LOG_INFO << "query nsqlookupd " << lookupd_url;
        std::shared_ptr<evpp::httpc::Request> r(new evpp::httpc::Request(this->loop_, lookupd_url, "", evpp::Duration(1.0)));
        r->Execute(std::bind(&Client::HandleLoopkupdHTTPResponse, this, std::placeholders::_1, r));
    };

    // query nsqloopupd immediately right now
    loop_->RunInLoop(f);

    // query nsqloopupd periodic
    loop_->RunEvery(option_.query_nsqlookupd_interval, f);
}

void Client::ConnectToLoopupds(const std::string& lookupd_urls/*http://192.168.0.5:4161/lookup?topic=test,http://192.168.0.6:4161/lookup?topic=test*/) {
    std::vector<std::string> v;
    evpp::StringSplit(lookupd_urls, ",", 0, v);
    for (auto it = v.begin(); it != v.end(); ++it) {
        ConnectToLoopupd(*it);
    }
}


void Client::Close() {
    auto f = [this]() {
        for (auto it = this->conns_.begin(), ite = this->conns_.end(); it != ite; ++it) {
            (*it)->Close();
        }

        for (auto it = this->connecting_conns_.begin(), ite = this->connecting_conns_.end(); it != ite; ++it) {
            it->second->Close();
        }
    };
    loop_->RunInLoop(f);
}

void Client::HandleLoopkupdHTTPResponse(
    const std::shared_ptr<evpp::httpc::Response>& response,
    const std::shared_ptr<evpp::httpc::Request>& request) {

    std::string body = response->body().ToString();
    if (response->http_code() != 200) {
        LOG_ERROR << "Request lookupd http://" << request->conn()->host() << ":"
                  << request->conn()->port() << request->uri()
                  << " failed, http-code=" << response->http_code()
                  << " [" << body << "]";
        return;
    }

    rapidjson::Document doc;
    doc.Parse(body.c_str());
    int status_code = doc["status_code"].GetInt();
    if (status_code != 200) {
        LOG_ERROR << "Request lookupd http://" << request->conn()->host()
                  << ":" << request->conn()->port() << request->uri()
                  << " failed: [" << body
                  << "]. We will automatically retry later.";
        return;
    } else {
        LOG_INFO << "lookupd response OK. http://"
                 << request->conn()->host() << ":"
                 << request->conn()->port() << request->uri()
                 << " : " << body;
    }

    rapidjson::Value& producers = doc["data"]["producers"];
    for (rapidjson::SizeType i = 0; i < producers.Size(); ++i) {
        rapidjson::Value& producer = producers[i];
        std::string broadcast_address = producer["broadcast_address"].GetString();
        int tcp_port = producer["tcp_port"].GetInt();
        std::string addr = broadcast_address + ":" + std::to_string(tcp_port);

        if (!IsKnownNSQDAddress(addr)) {
            ConnectToNSQD(addr);
        }
    }
}

void Client::OnConnection(const ConnPtr& conn) {
    if (conn->IsConnected() || conn->IsReady()) {
        conns_.push_back(conn);
        connecting_conns_.erase(conn->remote_addr());
        switch (conn->status()) {
        case NSQConn::kConnected:
            if (type_ == kConsumer) {
                conn->Subscribe(topic_, channel_);
            } else {
                assert(type_ == kProducer);
                conn->set_status(NSQConn::kReady);
                if (ready_to_publish_fn_) {
                    ready_to_publish_fn_(conn.get());
                }
            }
            break;
        case NSQConn::kReady:
            assert(type_ == kConsumer);
            break;
        default:
            break;
        }
    } else if (conn->IsConnecting()){
        MoveToConnectingList(conn);
    } else {
        // 应用层主动调用 Close
        // 删除该 NSQ 连接
        for (auto it = conns_.begin(), ite = conns_.end(); it != ite; ++it) {
            if (*it == conn) {
                conns_.erase(it);
                return;
            }
        }

        connecting_conns_.erase(conn->remote_addr());

        if (connecting_conns_.empty() && conns_.empty()) {
            close_fn_();
        }
    }
}

bool Client::IsKnownNSQDAddress(const std::string& addr) const {
    if (connecting_conns_.find(addr) != connecting_conns_.end()) {
        return true;
    }

    for (auto c : conns_) {
        if (c->remote_addr() == addr) {
            return true;
        }
    }

    return false;
}

void Client::MoveToConnectingList(const ConnPtr& conn) {
    ConnPtr& connecting_conn = connecting_conns_[conn->remote_addr()];
    if (connecting_conn.get()) {
        // This connection is already in the connecting list
        // so do not need to remove it from conns_
        return;
    }

    for (auto it = conns_.begin(), ite = conns_.end(); it != ite; ++it) {
        if (*it == conn) {
            connecting_conn = conn;
            conns_.erase(it);
            return;
        }
    }
}

}
