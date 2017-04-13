#pragma once

#include <memory>
#include <functional>

struct event_base;

namespace recipes {

class TimerEventWatcher;
class InvokeTimer;

class InvokeTimer {
public:
    typedef std::function<void()> Functor;

    // @brief Create a timer. When the timer is timeout, the functor f will
    //  be invoked automatically.
    // @param evloop - The EventLoop runs this timer
    // @param timeout - The timeout when the timer is invoked
    // @param f -
    InvokeTimer(struct event_base* evloop, double timeout_ms, const Functor& f);
    ~InvokeTimer();

    // It is thread safe.
    // Start this timer.
    void Start();

private:
    void OnTimerTriggered();

private:
    struct event_base* loop_;
    double timeout_ms_;
    Functor functor_;
    std::shared_ptr<TimerEventWatcher> timer_;
};

}
