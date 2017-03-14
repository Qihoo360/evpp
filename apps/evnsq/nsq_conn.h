#pragma once

#include <string>
#include <memory>
#include <atomic>
#include <map>
#include <list>

#include <evpp/timestamp.h>

#include "evnsq_export.h"
#include "option.h"
#include "message.h"
#include "command.h"

namespace evpp {
class EventLoop;
class TCPClient;
class TCPConn;
typedef std::shared_ptr<evpp::TCPClient> TCPClientPtr;
typedef std::shared_ptr<evpp::TCPConn> TCPConnPtr;
}

namespace evnsq {

class Client;

// http://nsq.io/clients/tcp_protocol_spec.html
// http://wiki.jikexueyuan.com/project/nsq-guide/tcp_protocol_spec.html

// The class NSQConn represents a connection with one NSQD server
class EVNSQ_EXPORT NSQConn : public std::enable_shared_from_this<NSQConn> {
public:
    enum Status {
        kDisconnected = 0,
        kConnecting = 1,
        kIdentifying = 2,
        kConnected = 3, // Successfully connected to NSQD
        kSubscribing = 4,
        kReady = 5, // Ready to produce messages to NSQD or consume messages from NSQD
        kDisconnecting = 6,
    };

    typedef std::function<void(const std::shared_ptr<NSQConn>& conn)> ConnectionCallback;
    typedef std::function<void(const CommandPtr& cmd, bool successfull)> PublishResponseCallback;
public:
    NSQConn(Client* c, const Option& ops);
    ~NSQConn();
    void Connect(const std::string& nsqd_tcp_addr/*host:port*/);
    void Close();

    void SetMessageCallback(const MessageCallback& cb) {
        msg_fn_ = cb;
    }
    void SetConnectionCallback(const ConnectionCallback& cb) {
        conn_fn_ = cb;
    }
    void SetPublishResponseCallback(const PublishResponseCallback& cb) {
        publish_response_cb_ = cb;
    }
    bool WritePublishCommand(const CommandPtr& cmd);
    void WriteBinaryCommand(evpp::Buffer* buf);
    void Subscribe(const std::string& topic, const std::string& channel);

    void set_status(Status s) {
        status_ = s;
    }
    Status status() const {
        return status_;
    }
    bool IsReady() const {
        return status_ == kReady;
    }
    bool IsConnected() const {
        return status_ == kConnected;
    }
    bool IsConnecting() const {
        return status_ == kConnecting;
    }
    bool IsDisconnected() const {
        return status_ == kDisconnected;
    }
    const std::string& remote_addr() const;
private:
    void WriteCommand(const Command& cmd);
    void Reconnect();
    void OnTCPConnectionEvent(const evpp::TCPConnPtr& conn);
    void OnRecv(const evpp::TCPConnPtr& conn, evpp::Buffer* buf);
    void OnMessage(size_t message_len, int32_t frame_type, evpp::Buffer* buf);
    void Identify();
    void Finish(const std::string& id);
    void Requeue(const std::string& id);
    void UpdateReady(int count);
    void OnPublishResponse(const char* d, size_t len);
    void PushWaitACKCommand(const CommandPtr& cmd);
    CommandPtr PopWaitACKCommand();
private:
    Client* nsq_client_;
    evpp::EventLoop* loop_;
    Option option_;
    Status status_;
    evpp::TCPClientPtr tcp_client_;
    MessageCallback msg_fn_;
    ConnectionCallback conn_fn_;
    PublishResponseCallback publish_response_cb_;

    std::list<CommandPtr> wait_ack_; // FIXME XXX TODO FIX std::list::size() performance problem
    int64_t published_count_;
    int64_t published_ok_count_;
    int64_t published_failed_count_;

};
}

