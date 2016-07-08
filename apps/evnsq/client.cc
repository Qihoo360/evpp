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

    Client::Client(evpp::EventLoop* loop, Type t, const std::string& topic, const std::string& channel, const Option& ops)
        : loop_(loop), type_(t), option_(ops), topic_(topic), channel_(channel) {}

    Client::~Client() {}

    void Client::ConnectToNSQD(const std::string& addr) {
        auto c = ConnPtr(new Conn(loop_, option_));
        conns_[addr] = c;
        c->SetMessageCallback(msg_fn_);
        c->SetConnectionCallback(std::bind(&Client::OnConnection, this, std::placeholders::_1));
        c->Connect(addr);
    }

    void Client::ConnectToNSQDs(const std::string& addrs/*host1:port1,host2:port2*/) {
        std::vector<std::string> v;
        evpp::StringSplit(addrs, ",", 0, v);
        auto it = v.begin();
        auto ite = v.end();
        for (; it != ite; ++it) {
            ConnectToNSQD(*it);
        }
    }

    void Client::ConnectToLoopupd(const std::string& lookupd_url/*http://127.0.0.1:4161/lookup?topic=test*/) {
        std::shared_ptr<evpp::httpc::Request> r(new evpp::httpc::Request(loop_, lookupd_url, "", evpp::Duration(1.0)));
        r->Execute(std::bind(&Client::HandleLoopkupdHTTPResponse, this, std::placeholders::_1, r));
        //TODO Add a timer to query lookupd periodically
    }

    void Client::ConnectToLoopupds(const std::string& lookupd_urls/*http://192.168.0.5:4161/lookup?topic=test,http://192.168.0.6:4161/lookup?topic=test*/) {
        std::vector<std::string> v;
        evpp::StringSplit(lookupd_urls, ",", 0, v);
        auto it = v.begin();
        auto ite = v.end();
        for (; it != ite; ++it) {
            ConnectToLoopupd(*it);
        }
    }

    void Client::HandleLoopkupdHTTPResponse(
        const std::shared_ptr<evpp::httpc::Response>& response,
        const std::shared_ptr<evpp::httpc::Request>& request) {
        std::string body = response->body().ToString();
        rapidjson::Document doc;
        doc.Parse(body.c_str());
        int status_code = doc["status_code"].GetInt();
        if (status_code != 200) {
            LOG_ERROR << "lookupd http://" << request->conn()->host() << ":" << request->conn()->port() << request->uri() << " response failed: " << body;
            //TODO retry??
            return;
        } else {
            LOG_INFO << "lookupd response OK. http://" << request->conn()->host() << ":" << request->conn()->port() << request->uri() << " : " << body;
        }

        rapidjson::Value& producers = doc["data"]["producers"];
        for (rapidjson::SizeType i = 0; i < producers.Size(); ++i) {
            rapidjson::Value& producer = producers[i];
            std::string broadcast_address = producer["broadcast_address"].GetString();
            int tcp_port = producer["tcp_port"].GetInt();
            std::string addr = broadcast_address + ":" + evpp::cast(tcp_port);
            if (conns_.find(addr) == conns_.end()) {
                ConnectToNSQD(addr);
            }
        }
    }

    void Client::OnConnection(Conn* conn) {
        assert(conn->IsConnected());
        if (type_ == kConsumer) {
            conn->Subscribe(topic_, channel_);
        } else {
            conn->set_status(Conn::kReady);
        }

        if (ready_fn_) {
            ready_fn_();
        }
    }

    void Client::Subscribe() {
        auto it = conns_.begin();
        auto ite = conns_.end();
        for (; it != ite; ++it) {
            Command c;
            c.Subscribe(topic_, channel_);
            it->second->WriteCommand(c);
        }
    }

    void Client::UpdateReady(int count) {
        auto it = conns_.begin();
        auto ite = conns_.end();
        for (; it != ite; ++it) {
            Command c;
            c.Ready(count);
            it->second->WriteCommand(c);
        }
    }
}
