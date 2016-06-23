#pragma once

#include <string>
#include <memory>
#include <atomic>

#include <evpp/exp.h>
#include <evpp/tcp_callbacks.h>

#include "evnsq_export.h"
#include "option.h"
#include "message.h"

namespace evpp {
    class EventLoop;
    class TCPClient;
}

namespace evnsq {
    class Command;
    class EVNSQ_EXPORT Consumer {
    public:
        enum Status {
            kDisconnected = 0,
            kConnecting = 1,
            kSendingMagic = 2,
            kIdentifying = 3,
            kSubscribing = 4,
            kConnected = 5,
        };
        typedef std::function<void(const Message*)> MessageCallback;
    public:
        Consumer(evpp::EventLoop* loop, const std::string& topic, const std::string& channel, const Option& ops);
        ~Consumer();
        bool ConnectToNSQD(const std::string& addr);
        void SetMessageCallback(const MessageCallback& cb) { msg_fn_ = cb; }
    private:
        void OnConnection(const evpp::TCPConnPtr& conn);
        void OnRecv(const evpp::TCPConnPtr& conn, evpp::Buffer* buf, evpp::Timestamp ts);
    private:
        void OnMessage(size_t message_len, int32_t frame_type, evpp::Buffer* buf);
        void WriteCommand(const Command& c);
        void Identify();
        void Subscribe();
        void UpdateReady(int count);
    private:
        evpp::EventLoop* loop_;
        Option option_;
        Status status_;
        std::string topic_;
        std::string channel_;
        std::shared_ptr<evpp::TCPClient> tcpc_;
        MessageCallback msg_fn_;

//         int64_t messagesInFlight
//             maxRdyCount      int64
//             rdyCount         int64
//             lastRdyCount     int64
//             lastMsgTimestamp int64
    };
}

