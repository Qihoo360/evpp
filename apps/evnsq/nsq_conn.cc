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
    LOG_WARN << "NSQConn::Close() this=" << this << " status=" << StatusToString();
    status_ = kDisconnecting;
    assert(loop_->IsInLoopThread());
    tcp_client_->Disconnect();
}

void NSQConn::Reconnect() {
    LOG_WARN << "NSQConn::Close() this=" << this << " status=" << StatusToString() << " remote_nsq_addr=" << remote_addr();

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
    LOG_INFO << "NSQConn::OnTCPConnectionEvent status=" << StatusToString() << " TCPConn=" << conn.get() << " remote_addr=" << conn->remote_addr();
    if (conn->IsConnected()) {
        assert(tcp_client_->conn() == conn);
        if (status_ == kConnecting) {
            Identify();
        } else {
            // Maybe the user layer has close this NSQConn and then the underlying TCPConn established a connection with NSQD to invoke this callback
            assert(status_ == kDisconnecting);
        }
    } else {
        if (tcp_client_->auto_reconnect()) {
            // tcp_client_ will reconnect to remote NSQD again automatically
            status_ = kConnecting;
        } else {
            // the user layer close the connection
            assert(status_ == kDisconnecting);
            status_ = kDisconnected;
        }

        if (conn_fn_) {
            conn_fn_(shared_from_this());
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

        size_t body_len = size - sizeof(frame_type); // The message body length

        switch (status_) {
        case evnsq::NSQConn::kDisconnected:
            break;

        case evnsq::NSQConn::kConnecting:
            break;

        case evnsq::NSQConn::kIdentifying:
            if (option_.feature_negotiation) {
                /*
                    {
                        "max_rdy_count": 2500,
                        "version": "0.3.8",
                        "max_msg_timeout": 900000,
                        "msg_timeout": 60000,
                        "tls_v1": false,
                        "deflate": false,
                        "deflate_level": 0,
                        "max_deflate_level": 6,
                        "snappy": false,
                        "sample_rate": 0,
                        "auth_required": true,
                        "output_buffer_size": 16384,
                        "output_buffer_timeout": 250
                    }
                */
                std::string msg = buf->NextString(body_len);
                rapidjson::Document doc;
                doc.Parse(msg.data());
                if (doc.HasParseError()) {
                    LOG_ERROR << "Identify Response JSON parsed ERROR. rapidjson ERROR code=" << doc.GetParseError();
                    OnConnectedFailed();
                }
                bool auth_required = doc["auth_required"].GetBool();
                if (auth_required) {
                    Authenticate();
                    // TODO store the options responded of the Identify
                } else {
                    OnConnectedOK();
                }
            } else {
                if (buf->NextString(body_len) == kOK) {
                    OnConnectedOK();
                } else {
                    LOG_ERROR << "Identify ERROR";
                    OnConnectedFailed();
                }
            }

            break;

        case evnsq::NSQConn::kAuthenticating:
        {
            std::string msg = buf->NextString(body_len);
            if (msg.substr(0, 2) == "E_") {
                LOG_ERROR << "Authenticate Failed. [" << msg << "]";
                OnConnectedFailed();
            } else {
                rapidjson::Document doc;
                doc.Parse(msg.data());
                if (doc.HasParseError()) {
                    LOG_ERROR << "Identify Response JSON parsed ERROR. rapidjson ERROR code=" << doc.GetParseError();
                    OnConnectedFailed();
                } else {
                    /*
                        {
                            "identity": "9a45d3df376d999c7c37b2766ae4113bb4463a09",
                            "identity_url": "",
                            "permission_count": 1
                        }
                    */
                    OnConnectedOK();
                }
            }

            break;
        }

        case evnsq::NSQConn::kConnected:
            assert(false && "It should never come here.");
            break;

        case evnsq::NSQConn::kSubscribing:
            if (buf->NextString(body_len) == kOK) {
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
            OnMessage(body_len, frame_type, buf);
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
    {
        // E_UNAUTHORIZED AUTH failed for PUB on "xyyyy1" ""
        std::string msg = std::string(buf->data(), message_len);
        LOG_ERROR << "frame_type=" << frame_type << " kFrameTypeResponse. [" << msg << "]";
        static const std::string unauthorized = "E_UNAUTHORIZED AUTH";
        if (strncmp(msg.data(), unauthorized.data(), unauthorized.size()) == 0) {
            Close();
            break;
        }

        if (status_ != kDisconnecting) {
            Reconnect();
        }
        break;
    }

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

void NSQConn::OnConnectedOK() {
    status_ = kConnected;
    if (conn_fn_) {
        auto self = shared_from_this();
        conn_fn_(self);
    }
}

void NSQConn::OnConnectedFailed() {
    Close();
}

void NSQConn::Authenticate() {
    Command c;
    c.Auth(option_.auth_secret);
    WriteCommand(c);
    status_ = kAuthenticating;
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

const char* NSQConn::StatusToString() const {
    H_CASE_STRING_BIGIN(status_);
    H_CASE_STRING(kDisconnected);
    H_CASE_STRING(kConnecting);
    H_CASE_STRING(kIdentifying);
    H_CASE_STRING(kAuthenticating);
    H_CASE_STRING(kConnected);
    H_CASE_STRING(kSubscribing);
    H_CASE_STRING(kReady);
    H_CASE_STRING(kDisconnecting);
    H_CASE_STRING_END();
}

void NSQConn::PushWaitACKCommand(const CommandPtr& cmd) {
    wait_ack_.push_back(cmd);
    published_count_++;
}

void NSQConn::OnPublishResponse(const char* d, size_t len) {
    CommandPtr cmd = PopWaitACKCommand();
    if (len == 2 && d[0] == 'O' && d[1] == 'K') {
        published_ok_count_++;
        LOG_INFO << "Get a PublishResponse message 'OK', command=" << cmd.get() << " published_ok_count=" << published_ok_count_;
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
