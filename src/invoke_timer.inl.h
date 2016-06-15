#pragma once

#include "evpp/inner_pre.h"

#include "evpp/event_loop.h"
#include "evpp/libevent_watcher.h"

#include <thread>

namespace evpp {
    class InvokeTimer
        : public std::enable_shared_from_this<InvokeTimer> {
    public:
        typedef std::function<void()> Functor;

        static std::shared_ptr<InvokeTimer> Create(EventLoop* evloop,
            Duration timeout, const Functor& f) {
            std::shared_ptr<InvokeTimer> it(new InvokeTimer(evloop, timeout, f));
            return it;
        }

        ~InvokeTimer() {
            LOG_INFO << "InvokeTimer::~InvokeTimer tid=" <<  std::this_thread::get_id();
            if (timer_) {
                delete timer_;
                timer_ = NULL;
            }
        }

        void Start() {
            std::shared_ptr<InvokeTimer> ref = shared_from_this(); // reference count +1
            LOG_INFO << "InvokeTimer::Start tid=" << std::this_thread::get_id() << " this=" << this << " refcount=" << ref.use_count();
            loop_->RunInLoop(std::bind(&InvokeTimer::AsyncWait, ref, timeout_));
        }

    private:
        InvokeTimer(EventLoop* evloop, Duration timeout, const Functor& f)
            : loop_(evloop), timeout_(timeout), functor_(f), timer_(NULL) {}

        void AsyncWait(Duration timeout) {
            std::shared_ptr<InvokeTimer> ref = shared_from_this(); // reference count +1
            LOG_INFO << "InvokeTimer::AsyncWait tid=" << std::this_thread::get_id() << " this=" << this << " refcount=" << ref.use_count();
            timer_ = new TimerEventWatcher(loop_->event_base(), std::bind(&InvokeTimer::OnTimeout, ref), timeout_);
            timer_->Init();
            timer_->AsyncWait();
        }

        void OnTimeout() {
            LOG_INFO << "InvokeTimer::OnTimeout tid=" << std::this_thread::get_id() << " this=" << this;
            functor_();
        }

    private:
        EventLoop* loop_;
        Duration timeout_;
        Functor functor_;
        TimerEventWatcher* timer_;
    };
}
