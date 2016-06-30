#include "consumer.h"

#include <evpp/event_loop.h>
#include <evpp/tcp_client.h>
#include <evpp/tcp_conn.h>
#include <evpp/httpc/request.h>
#include <evpp/httpc/response.h>
#include <evpp/httpc/conn.h>

#include <rapidjson/document.h>

#include "command.h"
#include "option.h"

namespace evnsq {
    static const std::string kNSQMagic = "  V2";
    static const std::string kOK = "OK";

    Consumer::Consumer(evpp::EventLoop* loop, const std::string& topic, const std::string& channel, const Option& ops)
        : loop_(loop), option_(ops), topic_(topic), channel_(channel) {}

    Consumer::~Consumer() {}

    void Consumer::ConnectToNSQD(const std::string& addr) {
        auto c = evpp::TCPClientPtr(new evpp::TCPClient(loop_, addr, std::string("NSQClient-")+addr));
        auto tc = NSQTCPClient(c, kConnecting);
        conns_[addr] = tc;
        c->SetConnectionCallback(std::bind(&Consumer::OnConnection, this, std::placeholders::_1));
        c->SetMessageCallback(std::bind(&Consumer::OnRecv, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
        c->Connect();
    }

    void Consumer::ConnectToNSQDs(const std::string& addrs/*host1:port1,host2:port2*/) {
        std::vector<std::string> v;
        evpp::StringSplit(addrs, ",", 0, v);
        auto it = v.begin();
        auto ite = v.end();
        for (; it !=ite; ++it) {
            ConnectToNSQD(*it);
        }
    }

    void Consumer::ConnectToLoopupd(const std::string& lookupd_url/*http://127.0.0.1:4161/lookup?topic=test*/) {
        std::shared_ptr<evpp::httpc::Request> r(new evpp::httpc::Request(loop_, lookupd_url, "", evpp::Duration(1.0)));
        r->Execute(std::bind(&Consumer::HandleLoopkupdHTTPResponse, this, std::placeholders::_1, r));
        //TODO Add a timer to query lookupd periodically
    }

    void Consumer::ConnectToLoopupds(const std::string& lookupd_urls/*http://192.168.0.5:4161/lookup?topic=test,http://192.168.0.6:4161/lookup?topic=test*/) {
        std::vector<std::string> v;
        evpp::StringSplit(lookupd_urls, ",", 0, v);
        auto it = v.begin();
        auto ite = v.end();
        for (; it != ite; ++it) {
            ConnectToLoopupd(*it);
        }
    }

    void Consumer::HandleLoopkupdHTTPResponse(
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

    void Consumer::OnConnection(const evpp::TCPConnPtr& conn) {
        if (conn->IsConnected()) {
            auto it = conns_.find(conn->remote_addr());
            assert(it != conns_.end());
            assert(it->second.c->conn() == conn);
            assert(it->second.s == kConnecting);
            Identify(it->second);
        } else {
            if (conn->IsDisconnecting()) {
                LOG_ERROR << "Connection to " << conn->remote_addr() << " was closed by remote server.";
            } else {
                LOG_ERROR << "Connect to " << conn->remote_addr() << " failed.";
            }
        }
    }

    void Consumer::OnRecv(const evpp::TCPConnPtr& conn, evpp::Buffer* buf, evpp::Timestamp ts) {
        if (buf->size() < 4) {
            return;
        }

        size_t size = buf->PeekInt32();
        if (buf->size() < size) {
            // need to read more data
            return;
        }
        auto it = conns_.find(conn->remote_addr());
        assert(it != conns_.end());
        buf->Skip(4); // 4 bytes of size
        //LOG_INFO << "Recv a data from NSQD msg body len=" << size - 4 << " body=[" << std::string(buf->data(), size - 4) << "]";
        int32_t frame_type = buf->ReadInt32();
        switch (it->second.s) {
        case evnsq::Consumer::kDisconnected:
            break;
        case evnsq::Consumer::kConnecting:
            break;
        case evnsq::Consumer::kIdentifying:
            if (buf->NextString(size - sizeof(frame_type)) == kOK) {
                Subscribe(it->second);
            } else {
                //TODO
            }
            break;
        case evnsq::Consumer::kSubscribing:
            if (buf->NextString(size - sizeof(frame_type)) == kOK) {
                it->second.s = kConnected;
                LOG_INFO << "Successfully connected to nsqd " << conn->remote_addr();
                UpdateReady(it->second, 3); //TODO RDY count
            } else {
                //TODO
            }
            break;
        case evnsq::Consumer::kConnected:
            OnMessage(it->second, size - sizeof(frame_type), frame_type, buf);
            break;
        default:
            break;
        }
    }

    void Consumer::OnMessage(const NSQTCPClient& tc, size_t message_len, int32_t frame_type, evpp::Buffer* buf) {
        if (frame_type == kFrameTypeResponse) {
            if (strncmp(buf->data(), "_heartbeat_", 11) == 0) {
                LOG_TRACE << "recv heartbeat from nsqd " << tc.c->remote_addr();
                Command c;
                c.Nop();
                WriteCommand(tc, c);
            } else {
                LOG_ERROR << "frame_type=" << frame_type << " kFrameTypeResponse. [" << std::string(buf->data(), message_len) << "]";
            }
            buf->Skip(message_len);
            return;
        }

        if (frame_type == kFrameTypeMessage) {
            Message msg;
            msg.Decode(message_len, buf);
            if (msg_fn_) {
                //TODO dispatch msg to a working thread pool
                if (msg_fn_(&msg) == 0) {
                    Finish(tc, msg.id);
                } else {
                    Requeue(tc, msg.id);
                }
            }
            return;
        }

        if (frame_type == kFrameTypeError) {
            //TODO add error processing logic
        }
    }

    void Consumer::WriteCommand(const NSQTCPClient& tc, const Command& c) {
        evpp::Buffer buf;
        c.WriteTo(&buf);
        tc.c->conn()->Send(&buf);
    }

    void Consumer::Identify(NSQTCPClient& tc) {
        tc.c->conn()->Send(kNSQMagic);
        Command c;
        c.Identify(option_.ToJSON());
        WriteCommand(tc, c);
        tc.s = kIdentifying;
    }

    void Consumer::Subscribe(NSQTCPClient& tc) {
        Command c;
        c.Subscribe(topic_, channel_);
        WriteCommand(tc, c);
        tc.s = kSubscribing;
    }

    void Consumer::UpdateReady(const NSQTCPClient& tc, int count) {
        Command c;
        c.Ready(count);
        WriteCommand(tc, c);
    }


    void Consumer::Finish(const NSQTCPClient& tc, const std::string& id) {
        Command c;
        c.Finish(id);
        WriteCommand(tc, c);
    }

    void Consumer::Requeue(const NSQTCPClient& tc, const std::string& id) {
        Command c;
        c.Requeue(id, evpp::Duration(0));
        WriteCommand(tc, c);
    }
}
