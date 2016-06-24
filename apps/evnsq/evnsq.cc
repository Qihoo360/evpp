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
        : loop_(loop), option_(ops), status_(kDisconnected), topic_(topic), channel_(channel) {}

    Consumer::~Consumer() {}

    bool Consumer::ConnectToNSQD(const std::string& addr) {
        tcpc_.reset(new evpp::TCPClient(loop_, addr, "NSQClient"));
        tcpc_->SetConnectionCallback(std::bind(&Consumer::OnConnection, this, std::placeholders::_1));
        tcpc_->SetMessageCallback(std::bind(&Consumer::OnRecv, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
        tcpc_->Connect();
        status_ = kConnecting;
        return false;
    }

    void Consumer::OnConnection(const evpp::TCPConnPtr& conn) {
        if (conn->IsConnected()) {
            assert(kConnecting);
            status_ = kSendingMagic;
            conn->Send(kNSQMagic);
        } else {
            //TODO
            LOG_ERROR << "Connect to " << tcpc_->remote_addr() << " failed.";
        }
    }

    void Consumer::OnRecv(const evpp::TCPConnPtr& conn, evpp::Buffer* buf, evpp::Timestamp ts) {
        if (buf->size() < 4) {
            return;
        }

        size_t size = buf->PeekInt32();
        if (size > buf->size()) {
            // need to read more data
            return;
        }
        buf->Skip(sizeof int32_t);
        int32_t frame_type = buf->ReadInt32();
        switch (status_) {
        case evnsq::Consumer::kDisconnected:
            break;
        case evnsq::Consumer::kConnecting:
            break;
        case evnsq::Consumer::kSendingMagic:
            if (buf->NextString(size - sizeof(frame_type)) == kOK) {
                Identify();
            } else {
                //TODO
            }
            break;
        case evnsq::Consumer::kIdentifying:
            if (buf->NextString(size - sizeof(frame_type)) == kOK) {
                Subscribe();
            } else {
                //TODO
            }
            break;
        case evnsq::Consumer::kSubscribing:
            if (buf->NextString(size - sizeof(frame_type)) == kOK) {
                status_ = kConnected;
                UpdateReady(1);
            } else {
                //TODO
            }
            break;
        case evnsq::Consumer::kConnected:
            OnMessage(size - sizeof(frame_type), frame_type, buf);
            break;
        default:
            break;
        }
    }

    void Consumer::OnMessage(size_t message_len, int32_t frame_type, evpp::Buffer* buf) {
        Message msg;
        msg.Decode(message_len, buf);
        if (msg_fn_) {
            msg_fn_(&msg);
        }
    }

    void Consumer::WriteCommand(const Command& c) {
        evpp::Buffer buf;
        c.WriteTo(&buf);
        tcpc_->conn()->Send(&buf);
    }

    void Consumer::Identify() {
        Command c;
        c.Identify(option_.ToJSON());
        WriteCommand(c);
        status_ = kIdentifying;
    }

    void Consumer::Subscribe() {
        Command c;
        c.Subscribe(topic_, channel_);
        WriteCommand(c);
        status_ = kSubscribing;
    }

    void Consumer::UpdateReady(int count) {
        Command c;
        c.Ready(count);
        WriteCommand(c);
    }

}