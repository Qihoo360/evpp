#pragma once

#include "evpp/inner_pre.h"

#include <thread>

struct event_base;
struct event;

namespace evpp {

class EventLoop;
class EVPP_EXPORT EventLoopThread {
public:
    typedef std::function<void()> Functor;
    EventLoopThread();
    ~EventLoopThread();

    bool Start(bool wait_until_thread_started = false,
               const Functor& pre = Functor(),
               const Functor& post = Functor());
    void Stop(bool wait_thread_exit = false);

    void SetName(const std::string& n);
    const std::string& name() const;
    EventLoop* event_loop() const;
    struct event_base* event_base();
    std::thread::id tid() const;
    bool IsRunning() const;
    bool IsStopped() const;

    void AfterFork();

private:
    void Run(const Functor& pre, const Functor& post);

private:
    std::shared_ptr<EventLoop> event_loop_;
    std::shared_ptr<std::thread> thread_;
    enum Status {
        kRunning = 1,
        kStopping = 2,
        kStopped = 3,
    };
    Status status_;

    std::string name_;
};
}
