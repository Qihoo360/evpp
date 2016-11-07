#include "conn.h"

#include <evpp/event_loop.h>
#include <evpp/tcp_client.h>
#include <evpp/tcp_conn.h>
#include <evpp/httpc/request.h>
#include <evpp/httpc/response.h>
#include <evpp/httpc/conn.h>

#include <rapidjson/document.h>

#include "command.h"
#include "option.h"
#include "client.h"

namespace evnsq {
static const std::string kNSQMagic = "  V2";
static const std::string kOK = "OK";

Conn::Conn(Client* c, const Option& ops)
    : client_(c), loop_(c->loop()), option_(ops), status_(kDisconnected) {}

Conn::~Conn() {}

void Conn::Connect(const std::string& addr) {
    tcp_client_ = evpp::TCPClientPtr(new evpp::TCPClient(loop_, addr, std::string("NSQClient-") + addr));
    status_ = kConnecting;
    tcp_client_->SetConnectionCallback(std::bind(&Conn::OnTCPConnectionEvent, this, std::placeholders::_1));
    tcp_client_->SetMessageCallback(std::bind(&Conn::OnRecv, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    tcp_client_->Connect();
}

void Conn::Reconnect() {
    tcp_client_->Disconnect();
    Connect(tcp_client_->remote_addr());
}

void Conn::OnTCPConnectionEvent(const evpp::TCPConnPtr& conn) {
    if (conn->IsConnected()) {
        assert(tcp_client_->conn() == conn);
        assert(status_ == kConnecting);
        Identify();
    } else {
        status_ = kDisconnected;
        if (conn->IsDisconnecting()) {
            LOG_ERROR << "Connection to " << conn->remote_addr() << " was closed by remote server.";
        } else {
            LOG_ERROR << "Connect to " << conn->remote_addr() << " failed.";
        }

        if (conn_fn_) {
            conn_fn_(shared_from_this());
        }

        status_ = kConnecting; // tcp_client_ will reconnect again automatically
    }
}

void Conn::OnRecv(const evpp::TCPConnPtr& conn, evpp::Buffer* buf, evpp::Timestamp ts) {
    while (buf->size() > 4) {
        size_t size = buf->PeekInt32();

        if (buf->size() < size) {
            // need to read more data
            return;
        }

        buf->Skip(4); // 4 bytes of size
        //LOG_INFO << "Recv a data from NSQD msg body len=" << size - 4 << " body=[" << std::string(buf->data(), size - 4) << "]";
        int32_t frame_type = buf->ReadInt32();

        switch (status_) {
        case evnsq::Conn::kDisconnected:
            break;

        case evnsq::Conn::kConnecting:
            break;

        case evnsq::Conn::kIdentifying:
            if (buf->NextString(size - sizeof(frame_type)) == kOK) {
                status_ = kConnected;
                if (conn_fn_) {
                    conn_fn_(shared_from_this());
                }
            } else {
                LOG_ERROR << "Identify ERROR";
                Reconnect();
            }
            break;

        case evnsq::Conn::kConnected:
            assert(false && "It should never come here.");
            break;

        case evnsq::Conn::kSubscribing:
            if (buf->NextString(size - sizeof(frame_type)) == kOK) {
                status_ = kReady;
                if (conn_fn_) {
                    conn_fn_(shared_from_this());
                }
                LOG_INFO << "Successfully connected to nsqd " << conn->remote_addr();
                UpdateReady(100); //TODO RDY count
            } else {
                Reconnect();
            }
            break;

        case evnsq::Conn::kReady:
            OnMessage(size - sizeof(frame_type), frame_type, buf);
            break;

        default:
            break;
        }
    }
}

void Conn::OnMessage(size_t message_len, int32_t frame_type, evpp::Buffer* buf) {
    if (frame_type == kFrameTypeResponse) {
        const size_t kHeartbeatLen = sizeof("_heartbeat_") - 1;
        if (message_len == kHeartbeatLen && strncmp(buf->data(), "_heartbeat_", kHeartbeatLen) == 0) {
            LOG_TRACE << "recv heartbeat from nsqd " << tcp_client_->remote_addr();
            Command c;
            c.Nop();
            WriteCommand(&c);
            buf->Skip(message_len);
            return;
        }
    }

    switch (frame_type) {
    case kFrameTypeResponse:
        LOG_INFO << "frame_type=" << frame_type << " kFrameTypeResponse. [" << std::string(buf->data(), message_len) << "]";
        if (client_->IsProducer() && publish_response_cb_) {
            publish_response_cb_(buf->data(), message_len);
        }
        buf->Skip(message_len);
        break;

    case kFrameTypeMessage: {
        Message msg;
        msg.Decode(message_len, buf);
        if (msg_fn_) {
            //TODO do we need to dispatch this msg to a working thread pool?
            if (msg_fn_(&msg) == 0) {
                Finish(msg.id);
            } else {
                Requeue(msg.id);
            }
        }
        return;
    }

    case kFrameTypeError:
        LOG_ERROR << "frame_type=" << frame_type << " kFrameTypeResponse. [" << std::string(buf->data(), message_len) << "]";
        // TODO how to do this?
        buf->Skip(message_len);
        break;

    default:
        break;
    }
}

void Conn::WriteCommand(const Command* c) {
    // TODO : using a object pool to improve performance
    evpp::Buffer buf;
    c->WriteTo(&buf);
    tcp_client_->conn()->Send(&buf);
}


void Conn::Subscribe(const std::string& topic, const std::string& channel) {
    Command c;
    c.Subscribe(topic, channel);
    WriteCommand(&c);
    status_ = kSubscribing;
}

const std::string& Conn::remote_addr() const {
    return tcp_client_->remote_addr();
}

void Conn::Identify() {
    tcp_client_->conn()->Send(kNSQMagic);
    Command c;
    c.Identify(option_.ToJSON());
    WriteCommand(&c);
    status_ = kIdentifying;
}

void Conn::Finish(const std::string& id) {
    Command c;
    c.Finish(id);
    WriteCommand(&c);
}

void Conn::Requeue(const std::string& id) {
    Command c;
    c.Requeue(id, evpp::Duration(0));
    WriteCommand(&c);
}

void Conn::UpdateReady(int count) {
    Command c;
    c.Ready(count);
    WriteCommand(&c);
}

}
