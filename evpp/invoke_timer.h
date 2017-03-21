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
                                               const Functor& f,
                                               bool periodic);
    static std::shared_ptr<InvokeTimer> Create(EventLoop* evloop,
                                               Duration timeout,
                                               Functor&& f,
                                               bool periodic);
    ~InvokeTimer();
    void Start();
    void Cancel();
    void set_cancel_callback(const Functor& fn) {
        cancel_callback_ = fn;
    }
private:
    InvokeTimer(EventLoop* evloop, Duration timeout, const Functor& f, bool periodic);
    InvokeTimer(EventLoop* evloop, Duration timeout, Functor&& f, bool periodic);
    void OnTimerTriggered();
    void OnCanceled();
private:
    EventLoop* loop_;
    Duration timeout_;
    Functor functor_;
    Functor cancel_callback_;
    std::shared_ptr<TimerEventWatcher> timer_;
    bool periodic_;
    std::shared_ptr<InvokeTimer> self_; // Hold myself
};

typedef std::shared_ptr<InvokeTimer> InvokeTimerPtr;
}
