#pragma once

#include "evpp/inner_pre.h"

#include "evpp/event_loop.h"
#include "evpp/libevent_watcher.h"

#include <thread>

namespace evpp {
    class InvokeTimer
        : public xstd::enable_shared_from_this<InvokeTimer> {
    public:
        typedef xstd::function<void()> Functor;

        static xstd::shared_ptr<InvokeTimer> Create(EventLoop* evloop,
            double timeout_ms, const Functor& f) {
            xstd::shared_ptr<InvokeTimer> it(new InvokeTimer(evloop, timeout_ms, f));
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
            xstd::shared_ptr<InvokeTimer> ref = shared_from_this(); // reference count +1
            loop_->RunInLoop(xstd::bind(&InvokeTimer::AsyncWait, ref, timeout_us_));
        }

    private:
        InvokeTimer(EventLoop* evloop, double timeout_ms, const Functor& f)
            : loop_(evloop), timeout_us_((uint64_t)(timeout_ms * 1000)), functor_(f), timer_(NULL) {}

        void AsyncWait(uint64_t timeout_us) {
            //LOG_INFO << "InvokeTimer::AsyncWait tid=" << std::this_thread::get_id();
            xstd::shared_ptr<InvokeTimer> ref = shared_from_this(); // reference count +1
            timer_ = new TimerEventWatcher(loop_->event_base(),
                xstd::bind(&InvokeTimer::OnTimeout, ref));
            timer_->Init();
            timer_->AsyncWait(timeout_us);
        }

        void OnTimeout() {
            //LOG_INFO << "InvokeTimer::OnTimeout tid=" << std::this_thread::get_id();
            functor_();
        }

    private:
        EventLoop* loop_;
        uint64_t timeout_us_;
        Functor functor_;
        TimerEventWatcher* timer_;
    };
}
