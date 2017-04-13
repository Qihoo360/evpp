#pragma once

#include "evpp/inner_pre.h"
#include "evpp/duration.h"

namespace evpp {
class EventLoop;
class TimerEventWatcher;
class InvokeTimer;

typedef std::shared_ptr<InvokeTimer> InvokeTimerPtr;

class EVPP_EXPORT InvokeTimer : public std::enable_shared_from_this<InvokeTimer> {
public:
    typedef std::function<void()> Functor;

    // @brief Create a timer. When the timer is timeout, the functor f will
    //  be invoked automatically.
    // @param evloop - The EventLoop runs this timer
    // @param timeout - The timeout when the timer is invoked
    // @param f -
    // @param periodic - To indicate this timer whether it is a periodic timer.
    //  If it is true this timer will be automatically invoked periodic.
    // @return evpp::InvokeTimerPtr - The user layer can hold this shared_ptr
    //  and can cancel this timer at any time.
    static InvokeTimerPtr Create(EventLoop* evloop,
                                 Duration timeout,
                                 const Functor& f,
                                 bool periodic);
    static InvokeTimerPtr Create(EventLoop* evloop,
                                 Duration timeout,
                                 Functor&& f,
                                 bool periodic);
    ~InvokeTimer();

    // It is thread safe.
    // Start this timer.
    void Start();

    // Cancel the timer and the cancel_callback_ will be invoked.
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

}
