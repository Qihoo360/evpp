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

    // A PUB/MPUB command which has already been serialized.
    bool PublishBinaryCommand(evpp::Buffer* command_binary_buf);

    void SetReadyCallback(const ReadyCallback& cb) {
        ready_fn_ = cb;
    }
    void SetHighWaterMarkCallback(const HighWaterMarkCallback& cb, size_t mark);

public:
    size_t published_count() const {
        return published_count_;
    }
    size_t published_ok_count() const {
        return published_ok_count_;
    }
    size_t published_failed_count() const {
        return published_failed_count_;
    }
    size_t high_water_mark() const {
        return high_water_mark_;
    }

private:
    bool Publish(const CommandPtr& cmd);
    bool PublishInLoop(const CommandPtr& cmd);
    void OnPublishResponse(NSQConn* conn, const CommandPtr& cmd, bool successfull);
    void OnReady(NSQConn* conn);
    NSQConnPtr GetNextConn();
    void PrintStats();
private:
    size_t current_conn_index_; // current Conn position at Client::conns_
    ReadyCallback ready_fn_;
    size_t published_count_;
    size_t published_ok_count_;
    size_t published_failed_count_;

    // TODO add HWM relative logic code
    enum { kDefaultHighWaterMark = 1024 };
    bool hwm_triggered_; // The flag of high water mark
    HighWaterMarkCallback high_water_mark_fn_;
    size_t high_water_mark_; // The high water mark for message count. default value is kDefaultHighWaterMark
};


}

