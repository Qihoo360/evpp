#pragma once

#include "evpp/inner_pre.h"
#include "evpp/duration.h"

namespace evpp {
    class EventLoop;
    class TimerEventWatcher;
    class EVPP_EXPORT InvokeTimer : public std::enable_shared_from_this<InvokeTimer> {
    public:
        typedef std::function<void()> Functor;
        static std::shared_ptr<InvokeTimer> Create(EventLoop* evloop,
                                                   Duration timeout,
                                                   const Functor& f);
        ~InvokeTimer();
        void Start();
        void Cancel();
    private:
        InvokeTimer(EventLoop* evloop, Duration timeout, const Functor& f);
        void AsyncWait(Duration timeout);
        void OnTimeout();
        void OnCanceled();
    private:
        EventLoop* loop_;
        Duration timeout_;
        Functor functor_;
        TimerEventWatcher* timer_;
        std::shared_ptr<InvokeTimer> self_; // Hold myself
    };

    typedef std::shared_ptr<InvokeTimer> InvokeTimerPtr;
}
