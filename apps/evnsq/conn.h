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

    class EVNSQ_EXPORT Conn {
    public:
        enum Status {
            kDisconnected = 0,
            kConnecting = 1,
            kIdentifying = 2,
            kConnected = 3, // Successfully connected to NSQD
            kSubscribing = 4,
            kReady = 5, // Ready to do produce message to NSQD or consume message from NSQD
        };

        typedef std::function<void(Conn*)> ConnectionCallback;
    public:
        Conn(evpp::EventLoop* loop, const Option& ops);
        ~Conn();
        void Connect(const std::string& nsqd_tcp_addr/*host:port*/);
        void SetMessageCallback(const MessageCallback& cb) { msg_fn_ = cb; }
        void SetConnectionCallback(const ConnectionCallback& cb) { conn_fn_ = cb; }
        void WriteCommand(const Command& c);
        void Subscribe(const std::string& topic, const std::string& channel);

        void set_status(Status s) { status_ = s; }
        Status status() const { return status_; }
        bool IsReady() const { return status_ == kReady; }
        bool IsConnected() const { return status_ == kConnected; }
    private:
        void Reconnect();
        void OnConnection(const evpp::TCPConnPtr& conn);
        void OnRecv(const evpp::TCPConnPtr& conn, evpp::Buffer* buf, evpp::Timestamp ts);
        void OnMessage(size_t message_len, int32_t frame_type, evpp::Buffer* buf);
        void Identify();
        void Finish(const std::string& id);
        void Requeue(const std::string& id);
        void UpdateReady(int count);
    private:
        evpp::EventLoop* loop_;
        Option option_;
        Status status_;
        evpp::TCPClientPtr tcp_client_;
        MessageCallback msg_fn_;
        ConnectionCallback conn_fn_;

        //         int64_t messagesInFlight
        //             maxRdyCount      int64
        //             rdyCount         int64
        //             lastRdyCount     int64
        //             lastMsgTimestamp int64
    };
}

