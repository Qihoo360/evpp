#pragma once

#include <string>
#include <memory>
#include <atomic>
#include <map>

#include "conn.h"

namespace evnsq {
    class EVNSQ_EXPORT Client {
    public:
        enum Type {
            kNone = 0,
            kConsumer = 1,
            kProducer = 2,
        };
        Client(evpp::EventLoop* loop, Type t, const std::string& topic, const std::string& channel, const Option& ops);
        ~Client();
        void ConnectToNSQD(const std::string& tcp_addr/*host:port*/);
        void ConnectToNSQDs(const std::string& tcp_addrs/*host1:port1,host2:port2*/);
        void ConnectToLoopupd(const std::string& lookupd_url/*http://127.0.0.1:4161/lookup?topic=test*/);
        void ConnectToLoopupds(const std::string& lookupd_urls/*http://192.168.0.5:4161/lookup?topic=test,http://192.168.0.6:4161/lookup?topic=test*/);
        void SetMessageCallback(const MessageCallback& cb) { msg_fn_ = cb; }
//     private:
//         void OnConnection(const evpp::TCPConnPtr& conn);
//         void OnRecv(const evpp::TCPConnPtr& conn, evpp::Buffer* buf, evpp::Timestamp ts);
//     private:
//         void OnMessage(const NSQTCPClient& tc, size_t message_len, int32_t frame_type, evpp::Buffer* buf);
//         void WriteCommand(const NSQTCPClient& tc, const Command& c);
    protected:
        void HandleLoopkupdHTTPResponse(
            const std::shared_ptr<evpp::httpc::Response>& response,
            const std::shared_ptr<evpp::httpc::Request>& request);
        void OnConnection(Conn* conn);
        void Subscribe();
        void UpdateReady(int count);
    protected:
        evpp::EventLoop* loop_;
        Type type_;
        Option option_;
        std::string topic_;
        std::string channel_;
        std::map<std::string/*host:port*/, ConnPtr> conns_; // The TCP connections with NSQD
        MessageCallback msg_fn_;

        //         int64_t messagesInFlight
        //             maxRdyCount      int64
        //             rdyCount         int64
        //             lastRdyCount     int64
        //             lastMsgTimestamp int64
    };
}

