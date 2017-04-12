#pragma once

#include "evpp/inner_pre.h"

#include <future>
#include <thread>

struct event_base;
struct event;

namespace evpp {

class EventLoop;

class EVPP_EXPORT EventLoopThread {
public:
    enum { kOK = 0 };

    // Return 0 means OK, anything else means failed.
    typedef std::function<int()> Functor;

    EventLoopThread();
    ~EventLoopThread();

    // 
    // @param wait_thread_started - If it is true we will wait the thread totally started
    // @param pre
    // @param post
    bool Start(bool wait_thread_started = true,
               Functor pre = Functor(),
               Functor post = Functor());
    void Stop(bool wait_thread_exit = false);

    void SetName(const std::string& n);
    const std::string& name() const;
    EventLoop* loop() const;
    struct event_base* event_base();
    std::thread::id tid() const;
    bool IsRunning() const;
    bool IsStopped() const;

    void AfterFork();

private:
    void Run(const Functor& pre);

private:
    std::shared_ptr<EventLoop> event_loop_;
    std::shared_ptr<std::thread> thread_;

    std::promise<int> pre_task_promise_;
    std::packaged_task<int()> post_task_;

    enum Status {
        kStarting = 0,
        kRunning = 1,
        kStopping = 2,
        kStopped = 3,
    };

    Status status_ = kStopped;

    std::string name_;
};
}
