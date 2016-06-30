#pragma once

#include <string>
#include <memory>
#include <atomic>
#include <map>

#include <evpp/exp.h>
#include <evpp/tcp_callbacks.h>

#include "evnsq_export.h"
#include "option.h"
#include "message.h"

namespace evpp {
    class EventLoop;
    class TCPClient;
    typedef std::shared_ptr<evpp::TCPClient> TCPClientPtr;

    namespace httpc {
        class Request;
        class Response;
    }
}

namespace evnsq {
    class Command;
    class EVNSQ_EXPORT Conn {
    public:
        enum Status {
            kDisconnected = 0,
            kConnecting = 1,
            kIdentifying = 2,
            kConnected = 3,
        };

        // MessageCallback is the message processing interface for Consumer
        //
        // Implement this interface for handlers that return whether or not message
        // processing completed successfully.
        //
        // When the return value is 0 Consumer will automatically handle FINishing.
        //
        // When the returned value is non-zero Consumer will automatically handle REQueing.
        typedef std::function<int(const Message*)> MessageCallback;

        struct NSQTCPClient {
            evpp::TCPClientPtr c;
            Status s;

            NSQTCPClient() : s(kDisconnected) {}
            NSQTCPClient(const evpp::TCPClientPtr& client) : c(client), s(kDisconnected) {}
            NSQTCPClient(const evpp::TCPClientPtr& client, Status status) : c(client), s(status) {}
        };
    public:
        Conn(evpp::EventLoop* loop, const Option& ops);
        ~Conn();
        void ConnectToNSQD(const std::string& tcp_addr/*host:port*/);
        void SetMessageCallback(const MessageCallback& cb) { msg_fn_ = cb; }
    private:
        void OnConnection(const evpp::TCPConnPtr& conn);
        void OnRecv(const evpp::TCPConnPtr& conn, evpp::Buffer* buf, evpp::Timestamp ts);
    private:
        void OnMessage(size_t message_len, int32_t frame_type, evpp::Buffer* buf);
        void WriteCommand(const Command& c);
    private:
        void Identify();
        void Finish(const std::string& id);
        void Requeue(const std::string& id);
    private:
        evpp::EventLoop* loop_;
        Option option_;
        Status status_;
        evpp::TCPClientPtr tcp_client_;
        MessageCallback msg_fn_;

        //         int64_t messagesInFlight
        //             maxRdyCount      int64
        //             rdyCount         int64
        //             lastRdyCount     int64
        //             lastMsgTimestamp int64
    };
}

