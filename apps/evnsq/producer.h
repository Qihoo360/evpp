#pragma once

#include <list>
#include <vector>
#include "client.h"
#include "command.h"

namespace evnsq {
class Command;
class EVNSQ_EXPORT Producer : public Client {
public:
    // When the connection to NSQD is ready, this callback will be called.
    // After this callback, the application can publish message.
    typedef std::function<void()> ReadyCallback;
    typedef std::function<void(Producer*, size_t)> HighWaterMarkCallback;

    Producer(evpp::EventLoop* loop, const Option& ops);
    ~Producer();

    // Thread safe
    bool Publish(const std::string& topic, const std::string& msg);
    bool MultiPublish(const std::string& topic, const std::vector<std::string>& messages);

    void SetReadyCallback(const ReadyCallback& cb) {
        ready_fn_ = cb;
    }
    void SetHighWaterMarkCallback(const HighWaterMarkCallback& cb, size_t mark);
private:
    bool Publish(Command* cmd);
    void PublishInLoop(Command* cmd);
    void OnReady(Conn* conn);
    void OnPublishResponse(Conn* conn, const char* d, size_t len);
    void PushWaitACKCommand(Conn* conn, Command* cmd);
    Command* PopWaitACKCommand(Conn* conn);
    ConnPtr GetNextConn();
private:
    size_t current_conn_index_; // current Conn position at Client::conns_
    typedef std::pair<std::list<Command*>, size_t/*Command count*/> CommandList;
    std::map<Conn*, CommandList> wait_ack_;
    ReadyCallback ready_fn_;
    size_t wait_ack_count_;
    int64_t published_count_;
    int64_t published_ok_count_;
    int64_t published_failed_count_;

    enum { kDefaultHighWaterMark = 1024 };
    bool hwm_triggered_; // The flag of high water mark
    HighWaterMarkCallback high_water_mark_fn_;
    size_t high_water_mark_; // The default value is kDefaultHighWaterMark
};


}

