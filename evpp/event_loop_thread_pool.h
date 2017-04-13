#pragma once

#include "evpp/event_loop_thread.h"

#include <atomic>

namespace evpp {
class EVPP_EXPORT EventLoopThreadPool {
public:
    typedef std::function<void()> Functor;

    EventLoopThreadPool(EventLoop* base_loop, uint32_t thread_num);
    ~EventLoopThreadPool();

    bool Start(bool wait_thread_started = false);

    void Stop(bool wait_thread_exited = false);
    void Stop(Functor on_stopped_cb);

public:
    EventLoop* GetNextLoop();
    EventLoop* GetNextLoopWithHash(uint64_t hash);

    bool IsRunning() const;
    bool IsStopped() const;
    uint32_t thread_num() const;

private:
    void Stop(bool wait_thread_exit, Functor on_stopped_cb);
    void OnThreadStarted(uint32_t count);
    void OnThreadExited(uint32_t count);

private:
    EventLoop* base_loop_;
    std::atomic<bool> started_ = { false };
    uint32_t thread_num_ = 0;
    std::atomic<int64_t> next_ = { 0 };

    Functor stopped_cb_;
    std::promise<void> exit_promise_; // wait all threads exited

    typedef std::shared_ptr<EventLoopThread> EventLoopThreadPtr;
    std::vector<EventLoopThreadPtr> threads_;
};
}
