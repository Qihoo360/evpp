#include "nsq_conn.h"

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
#include "producer.h"

namespace evnsq {
static const std::string kNSQMagic = "  V2";
static const std::string kOK = "OK";

NSQConn::NSQConn(Client* c, const Option& ops)
    : nsq_client_(c)
    , loop_(c->loop())
    , option_(ops)
    , status_(kDisconnected)
      //, wait_ack_count_(0)
    , published_count_(0)
    , published_ok_count_(0)
    , published_failed_count_(0) {}

NSQConn::~NSQConn() {
}

void NSQConn::Connect(const std::string& addr) {
    tcp_client_ = evpp::TCPClientPtr(new evpp::TCPClient(loop_, addr, std::string("NSQClient-") + addr));
    status_ = kConnecting;
    tcp_client_->SetConnectionCallback(std::bind(&NSQConn::OnTCPConnectionEvent, this, std::placeholders::_1));
    tcp_client_->SetMessageCallback(std::bind(&NSQConn::OnRecv, this, std::placeholders::_1, std::placeholders::_2));
    tcp_client_->Connect();
}


void NSQConn::Close() {
    assert(loop_->IsInLoopThread());
    tcp_client_->Disconnect();
    status_ = kDisconnecting;
}

void NSQConn::Reconnect() {
    // Discards all the messages which were cached by the broken tcp connection.
    if (!wait_ack_.empty()) {
        LOG_WARN << "Discards " << wait_ack_.size() << " NSQ messages. nsq_message_missing";
        published_failed_count_ += wait_ack_.size();
        wait_ack_.clear();
    }

    tcp_client_->Disconnect();
    Connect(tcp_client_->remote_addr());
}

const std::string& NSQConn::remote_addr() const {
    return tcp_client_->remote_addr();
}

void NSQConn::OnTCPConnectionEvent(const evpp::TCPConnPtr& conn) {
    if (conn->IsConnected()) {
        assert(tcp_client_->conn() == conn);
        assert(status_ == kConnecting);
        Identify();
    } else {
        if (conn->IsDisconnecting()) {
            LOG_ERROR << "Connection to " << conn->remote_addr() << " was closed by remote server.";
        } else {
            LOG_ERROR << "Connect to " << conn->remote_addr() << " failed.";
        }

        if (tcp_client_->auto_reconnect()) {
            // tcp_client_ will reconnect to remote NSQD again automatically
            status_ = kConnecting;
        } else {
            // the user layer close the connection
            assert(status_ == kDisconnecting);
            status_ = kDisconnected;
        }

        if (conn_fn_) {
            auto self = shared_from_this();
            conn_fn_(self);
        }
    }
}

void NSQConn::OnRecv(const evpp::TCPConnPtr& conn, evpp::Buffer* buf) {
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
        case evnsq::NSQConn::kDisconnected:
            break;

        case evnsq::NSQConn::kConnecting:
            break;

        case evnsq::NSQConn::kIdentifying:
            if (buf->NextString(size - sizeof(frame_type)) == kOK) {
                status_ = kConnected;
                if (conn_fn_) {
                    auto self = shared_from_this();
                    conn_fn_(self);
                }
            } else {
                LOG_ERROR << "Identify ERROR";
                Reconnect();
            }
            break;

        case evnsq::NSQConn::kConnected:
            assert(false && "It should never come here.");
            break;

        case evnsq::NSQConn::kSubscribing:
            if (buf->NextString(size - sizeof(frame_type)) == kOK) {
                status_ = kReady;
                if (conn_fn_) {
                    auto self = shared_from_this();
                    conn_fn_(self);
                }
                LOG_INFO << "Successfully connected to nsqd " << conn->remote_addr();
                UpdateReady(100); //TODO RDY count
            } else {
                Reconnect();
            }
            break;

        case evnsq::NSQConn::kReady:
            OnMessage(size - sizeof(frame_type), frame_type, buf);
            break;

        default:
            break;
        }
    }
}

void NSQConn::OnMessage(size_t message_len, int32_t frame_type, evpp::Buffer* buf) {
    if (frame_type == kFrameTypeResponse) {
        const size_t kHeartbeatLen = sizeof("_heartbeat_") - 1;
        if (message_len == kHeartbeatLen && strncmp(buf->data(), "_heartbeat_", kHeartbeatLen) == 0) {
            LOG_TRACE << "recv heartbeat from nsqd " << tcp_client_->remote_addr();
            Command c;
            c.Nop();
            WriteCommand(c);
            buf->Skip(message_len);
            return;
        }
    }

    switch (frame_type) {
    case kFrameTypeResponse:
        LOG_INFO << "frame_type=" << frame_type << " kFrameTypeResponse. [" << std::string(buf->data(), message_len) << "]";
        if (nsq_client_->IsProducer()) {
            OnPublishResponse(buf->data(), message_len);
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
        if (status_ != kDisconnecting) {
            Reconnect();
        }
        break;

    default:
        break;
    }
}

void NSQConn::WriteCommand(const Command& c) {
    // TODO : using a object pool to improve performance
    evpp::Buffer buf;
    c.WriteTo(&buf);
    WriteBinaryCommand(&buf);
}

void NSQConn::Subscribe(const std::string& topic, const std::string& channel) {
    Command c;
    c.Subscribe(topic, channel);
    WriteCommand(c);
    status_ = kSubscribing;
}

void NSQConn::Identify() {
    tcp_client_->conn()->Send(kNSQMagic);
    Command c;
    c.Identify(option_.ToJSON());
    WriteCommand(c);
    status_ = kIdentifying;
}

void NSQConn::Finish(const std::string& id) {
    Command c;
    c.Finish(id);
    WriteCommand(c);
}

void NSQConn::Requeue(const std::string& id) {
    Command c;
    c.Requeue(id, evpp::Duration(0));
    WriteCommand(c);
}

void NSQConn::UpdateReady(int count) {
    Command c;
    c.Ready(count);
    WriteCommand(c);
}

bool NSQConn::WritePublishCommand(const CommandPtr& c) {
    assert(c->IsPublish());
    assert(nsq_client_->IsProducer());
    if (wait_ack_.size() >= static_cast<Producer*>(nsq_client_)->high_water_mark()) {
        LOG_EVERY_N(WARNING, 100000) << "Too many messages are waiting a response ACK. Please try again later.";
        return false;
    }

    // TODO : using a object pool to improve performance
    evpp::Buffer buf;
    c->WriteTo(&buf);
    WriteBinaryCommand(&buf);
    PushWaitACKCommand(c);
    LOG_INFO << "Publish a message to " << remote_addr() << " command=" << c.get();
    return true;
}


void NSQConn::WriteBinaryCommand(evpp::Buffer* buf) {
    tcp_client_->conn()->Send(buf);
}

CommandPtr NSQConn::PopWaitACKCommand() {
    if (wait_ack_.empty()) {
        return CommandPtr();
    }
    CommandPtr c = *wait_ack_.begin();
    wait_ack_.pop_front();
    return c;
}

void NSQConn::PushWaitACKCommand(const CommandPtr& cmd) {
    wait_ack_.push_back(cmd);
    published_count_++;
}

void NSQConn::OnPublishResponse(const char* d, size_t len) {
    CommandPtr cmd = PopWaitACKCommand();
    if (len == 2 && d[0] == 'O' && d[1] == 'K') {
        LOG_INFO << "Get a PublishResponse message 'OK', command=" << cmd.get();
        published_ok_count_++;
        publish_response_cb_(cmd, true);
        return;
    }

    LOG_ERROR << "Publish message failed : [" << std::string(d, len) << "].";
    if (!cmd.get()) {
        return;
    }

    published_failed_count_++;
    if (cmd->retried_time() >= 2) {
        publish_response_cb_(cmd, false);
        return;
    }

    cmd->IncRetriedTime();
    LOG_ERROR << "Publish command " << cmd.get() << " failed : [" << std::string(d, len) << "]. Try again.";
    WritePublishCommand(cmd); // TODO This code will serialize Command more than twice. We need to cache the first serialization result to fix this performance problem
}

}
