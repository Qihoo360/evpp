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
    class EVNSQ_EXPORT Consumer {
    public:
        enum Status {
            kDisconnected = 0,
            kConnecting = 1,
            kIdentifying = 2,
            kSubscribing = 3,
            kConnected = 4,
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
        Consumer(evpp::EventLoop* loop, const std::string& topic, const std::string& channel, const Option& ops);
        ~Consumer();
        void ConnectToNSQD(const std::string& tcp_addr/*host:port*/);
        void ConnectToNSQDs(const std::string& tcp_addrs/*host1:port1,host2:port2*/);
        void ConnectToLoopupd(const std::string& lookupd_url/*http://127.0.0.1:4161/lookup?topic=test*/);
        void ConnectToLoopupds(const std::string& lookupd_urls/*http://192.168.0.5:4161/lookup?topic=test,http://192.168.0.6:4161/lookup?topic=test*/);
        void SetMessageCallback(const MessageCallback& cb) { msg_fn_ = cb; }
    private:
        void OnConnection(const evpp::TCPConnPtr& conn);
        void OnRecv(const evpp::TCPConnPtr& conn, evpp::Buffer* buf, evpp::Timestamp ts);
    private:
        void OnMessage(const NSQTCPClient& tc, size_t message_len, int32_t frame_type, evpp::Buffer* buf);
        void WriteCommand(const NSQTCPClient& tc, const Command& c);
        void HandleLoopkupdHTTPResponse(
            const std::shared_ptr<evpp::httpc::Response>& response,
            const std::shared_ptr<evpp::httpc::Request>& request);
    private:
        void Identify(NSQTCPClient& tc);
        void Subscribe(NSQTCPClient& tc);
        void UpdateReady(const NSQTCPClient& tc, int count);
        void Finish(const NSQTCPClient& tc, const std::string& id);
        void Requeue(const NSQTCPClient& tc, const std::string& id);
    private:
        evpp::EventLoop* loop_;
        Option option_;
        std::string topic_;
        std::string channel_;
        std::map<std::string/*host:port*/, NSQTCPClient> conns_;
        MessageCallback msg_fn_;

        //         int64_t messagesInFlight
        //             maxRdyCount      int64
        //             rdyCount         int64
        //             lastRdyCount     int64
        //             lastMsgTimestamp int64
    };
}

