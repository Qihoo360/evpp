#pragma once

#include "evpp/event_loop_thread.h"

namespace evpp {
    class EVPP_EXPORT EventLoopThreadPool {
    public:
        EventLoopThreadPool(EventLoop* base_loop);
        ~EventLoopThreadPool();
        void SetThreadNum(int thread_num);
        bool Start(bool wait_until_thread_started = false);
        void Stop(bool wait_thread_exit = false);

    public:
        EventLoop* GetNextLoop();
        EventLoop* GetNextLoopWithHash(uint64_t hash);

        bool IsRunning() const;
        bool IsStopped() const;

    public:
        typedef xstd::shared_ptr<EventLoopThread> EventLoopThreadPtr;
        std::vector<EventLoopThreadPtr>* threads();

    private:
        class Impl;
        xstd::shared_ptr<Impl> impl_;
    };
}
