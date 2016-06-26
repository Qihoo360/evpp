#include "evnsq.h"

#include <evpp/event_loop.h>
#include <evpp/tcp_client.h>
#include <evpp/tcp_conn.h>
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
        buf->Skip(sizeof int32_t);
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
                LOG_INFO << "Successfully connected to NSQ : " << conn->remote_addr();
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
                LOG_TRACE << "recv heartbeat";
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