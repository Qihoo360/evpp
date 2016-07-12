#pragma once

#include <list>
#include "client.h"

namespace evnsq {
    class Command;
    class EVNSQ_EXPORT Producer : public Client {
    public:
        // When the connection to NSQD is ready, this callback will be called.
        typedef std::function<void()> ReadyCallback;

        Producer(evpp::EventLoop* loop, const Option& ops);
        ~Producer();
        void Publish(const std::string& topic, const std::string& msg);
        void SetReadyCallback(const ReadyCallback& cb) { ready_fn_ = cb; }
    private:
        void OnReady(Conn* conn);
        void OnPublishResponse(Conn* conn, const char* d, size_t len);
        Command* PopWaitACKCommand(Conn* conn);
        void PushWaitACKCommand(Conn* conn, Command* cmd);
    private:
        std::map<std::string/*host:port*/, ConnPtr>::iterator conn_;
        typedef std::pair<std::list<Command*>, size_t> CommandList;
        std::map<Conn*, CommandList> wait_ack_;
        ReadyCallback ready_fn_;
        size_t wait_ack_count_;
        size_t published_count_;
        size_t published_ok_count_;
        size_t published_failed_count_;
    };
}

