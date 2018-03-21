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
    : loop_(l), type_(t), option_(ops), closing_(false) {
    DLOG_TRACE;
}

Client::~Client() {
    DLOG_TRACE;
}

void Client::ConnectToNSQD(const std::string& addr) {
    auto c = NSQConnPtr(new NSQConn(this, option_));
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

void Client::ConnectToLookupd(const std::string& lookupd_url/*http://127.0.0.1:4161/lookup?topic=test*/) {
    auto f = [this, lookupd_url]() {
        LOG_INFO << "query nsqlookupd " << lookupd_url;
        std::shared_ptr<evpp::httpc::Request> r(new evpp::httpc::Request(this->loop_, lookupd_url, "", evpp::Duration(1.0)));
        r->Execute(std::bind(&Client::HandleLoopkupdHTTPResponse, this, std::placeholders::_1, r));
    };

    // query nsqlookupd immediately right now
    loop_->RunInLoop(f);

    // query nsqlookupd periodic
    auto timer = loop_->RunEvery(option_.query_nsqlookupd_interval, f);
    lookupd_timers_.push_back(timer);
}

void Client::ConnectToLookupds(const std::string& lookupd_urls/*http://192.168.0.5:4161/lookup?topic=test,http://192.168.0.6:4161/lookup?topic=test*/) {
    std::vector<std::string> v;
    evpp::StringSplit(lookupd_urls, ",", 0, v);
    for (auto it = v.begin(); it != v.end(); ++it) {
        ConnectToLookupd(*it);
    }
}

void Client::Close() {
    DLOG_TRACE << "conns_.size=" << conns_.size() << " connecting_conns_.size=" << connecting_conns_.size();
    closing_ = true;

    auto f = [this]() {
        ready_to_publish_fn_ = ReadyToPublishCallback();
        for (auto& it : conns_) {
            DLOG_TRACE << "Close connected NSQConn " << it.get() << it->remote_addr();
            it->Close();
        }

        for (auto& it : connecting_conns_) {
            DLOG_TRACE << "Close connecting NSQConn " << it.second.get() << it.second->remote_addr();
            it.second->Close();
        }

        for (auto& timer : lookupd_timers_) {
            timer->Cancel();
        }
        lookupd_timers_.clear();
    };

    // If we use loop_->RunInLoop(f), this may execute f in current loop
    // and then it will callback Client::OnConnection to release NSQConn object.
    // That will make the iterators in function f broken down.
    // So we use loop_->QueueInLoop(f) to delay the execution time of f to next loop.
    loop_->QueueInLoop(f);
}

bool Client::IsReady() const {
    if (conns_.empty()) {
        return false;
    }

    for (auto& it : conns_) {
        if (it->IsReady()) {
            return true;
        }
    }
    return false;
}

void Client::HandleLoopkupdHTTPResponse(
    const std::shared_ptr<evpp::httpc::Response>& response,
    const std::shared_ptr<evpp::httpc::Request>& request) {
    DLOG_TRACE;

    if (response.get() == nullptr) {
        LOG_ERROR << "Request lookupd http://" << request->host() << ":"
            << request->port() << request->uri()
            << " failed, response is null";

        return;
    }

    std::string body = response->body().ToString();
    if (response->http_code() != 200) {
        LOG_ERROR << "Request lookupd http://" << request->host() << ":"
                  << request->port() << request->uri()
                  << " failed, http-code=" << response->http_code()
                  << " [" << body << "]";
        return;
    }

    rapidjson::Document doc;
    doc.Parse(body.c_str());
    int status_code = doc["status_code"].GetInt();
    if (status_code != 200) {
        LOG_ERROR << "Request lookupd http://" << request->host()
                  << ":" << request->port() << request->uri()
                  << " failed: [" << body
                  << "]. We will automatically retry later.";
        return;
    } else {
        LOG_INFO << "lookupd response OK. http://"
                 << request->host() << ":"
                 << request->port() << request->uri()
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

void Client::OnConnection(const NSQConnPtr& conn) {
    DLOG_TRACE << " NSQConn remote_addr=" << conn->remote_addr() << " status=" << conn->StatusToString();
    assert(loop_->IsInLoopThread());

    switch (conn->status()) {
    case NSQConn::kConnecting:
        MoveToConnectingList(conn);
        break;
    case NSQConn::kConnected:
        if (type_ == kConsumer) {
            conn->Subscribe(topic_, channel_);
        } else {
            assert(type_ == kProducer);
            conn->set_status(NSQConn::kReady);
            conns_.push_back(conn);
            connecting_conns_.erase(conn->remote_addr());
            if (ready_to_publish_fn_) {
                ready_to_publish_fn_(conn.get());
            }
        }
        break;
    case NSQConn::kReady:
        assert(type_ == kConsumer);
        conns_.push_back(conn);
        connecting_conns_.erase(conn->remote_addr());
        break;
    default:
        // The application layer calls Close()
        assert(conn->IsDisconnected());

        // Delete this NSQConn
        for (auto it = conns_.begin(), ite = conns_.end(); it != ite; ++it) {
            if (*it == conn) {
                conns_.erase(it);
                break;
            }
        }
        connecting_conns_.erase(conn->remote_addr());

        if (connecting_conns_.empty() && conns_.empty()) {
            if (close_fn_) {
                close_fn_();
            }
        }

        // Waiting for the status of NSQConn to be stable, and then execute the function f in next loop
        auto f = [this, conn]() {
            assert(conn->IsDisconnected());
            if (!conn->IsDisconnected()) {
                LOG_ERROR << "NSQConn status is not kDisconnected : " << int(conn->status());
            }
        };
        loop_->QueueInLoop(f);
        break;
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

void Client::MoveToConnectingList(const NSQConnPtr& conn) {
    NSQConnPtr& connecting_conn = connecting_conns_[conn->remote_addr()];
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
