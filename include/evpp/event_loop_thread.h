#pragma once

#include "evpp/inner_pre.h"

#include <thread>

struct event_base;
struct event;

namespace evpp {

    class EventLoop;
    class EVPP_EXPORT EventLoopThread {
    public:
        typedef xstd::function<void()> Functor;
        EventLoopThread();
        ~EventLoopThread();

        bool Start(bool wait_until_thread_started = false, const Functor& pre = Functor(), const Functor& post = Functor());
        void Stop(bool wait_thread_exit = false);

        void SetName(const std::string& n);
        const std::string& name() const;
        std::thread::id tid() const;
    public:
        EventLoop* event_loop() const;
        struct event_base *event_base();

        bool IsRunning() const;
        bool IsStopped() const;
    private:

        class Impl;
        xstd::shared_ptr<Impl> impl_;
    };
}
