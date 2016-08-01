#pragma once

#include <string>
#include <memory>
#include <atomic>
#include <map>

#include <evpp/timestamp.h>

#include "evnsq_export.h"
#include "option.h"
#include "message.h"

namespace evpp {
class EventLoop;
class TCPClient;
class TCPConn;
typedef std::shared_ptr<evpp::TCPClient> TCPClientPtr;
typedef std::shared_ptr<evpp::TCPConn> TCPConnPtr;
}

namespace evnsq {

class Command;
class Client;
class EVNSQ_EXPORT Conn : public std::enable_shared_from_this<Conn> {
public:
    enum Status {
        kDisconnected = 0,
        kConnecting = 1,
        kIdentifying = 2,
        kConnected = 3, // Successfully connected to NSQD
        kSubscribing = 4,
        kReady = 5, // Ready to do produce message to NSQD or consume message from NSQD
    };

    typedef std::function<void(const std::shared_ptr<Conn>& conn)> ConnectionCallback;
    typedef std::function<void(const char* d, size_t len)> PublishResponseCallback;
public:
    Conn(Client* c, const Option& ops);
    ~Conn();
    void Connect(const std::string& nsqd_tcp_addr/*host:port*/);
    void SetMessageCallback(const MessageCallback& cb) {
        msg_fn_ = cb;
    }
    void SetConnectionCallback(const ConnectionCallback& cb) {
        conn_fn_ = cb;
    }
    void SetPublishResponseCallback(const PublishResponseCallback& cb) {
        publish_response_cb_ = cb;
    }
    void WriteCommand(const Command* c);
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
    const std::string& remote_addr() const;
private:
    void Reconnect();
    void OnTCPConnectionEvent(const evpp::TCPConnPtr& conn);
    void OnRecv(const evpp::TCPConnPtr& conn, evpp::Buffer* buf, evpp::Timestamp ts);
    void OnMessage(size_t message_len, int32_t frame_type, evpp::Buffer* buf);
    void Identify();
    void Finish(const std::string& id);
    void Requeue(const std::string& id);
    void UpdateReady(int count);
private:
    Client* client_;
    evpp::EventLoop* loop_;
    Option option_;
    Status status_;
    evpp::TCPClientPtr tcp_client_;
    MessageCallback msg_fn_;
    ConnectionCallback conn_fn_;
    PublishResponseCallback publish_response_cb_;
};
}

