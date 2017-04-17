#pragma once

#include <memory>
#include <functional>

struct event_base;

namespace recipes {

class TimerEventWatcher;
class InvokeTimer;

typedef std::shared_ptr<InvokeTimer> InvokeTimerPtr;

class InvokeTimer : public std::enable_shared_from_this<InvokeTimer> {
public:
    typedef std::function<void()> Functor;

    static InvokeTimerPtr Create(struct event_base* evloop,
                                 double timeout_ms,
                                 const Functor& f);

    ~InvokeTimer();

    void Start();

    void Cancel();

    void set_cancel_callback(const Functor& fn) {
        cancel_callback_ = fn;
    }
private:
    InvokeTimer(struct event_base* evloop, double timeout_ms, const Functor& f);
    void OnTimerTriggered();
    void OnCanceled();

private:
    struct event_base* loop_;
    double timeout_ms_;
    Functor functor_;
    Functor cancel_callback_;
    std::shared_ptr<TimerEventWatcher> timer_;
    std::shared_ptr<InvokeTimer> self_; // Hold myself
};

}
