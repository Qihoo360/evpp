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

    // @brief Create a timer. When the timer is timeout, the functor f will
    //  be invoked automatically.
    // @param evloop - The EventLoop runs this timer
    // @param timeout - The timeout when the timer is invoked
    // @param f -
    // @param periodic - To indicate this timer whether it is a periodic timer.
    //  If it is true this timer will be automatically invoked periodic.
    // @return evpp::InvokeTimerPtr - The user layer can hold this shared_ptr
    //  and can cancel this timer at any time.
    static InvokeTimerPtr Create(struct event_base* evloop,
                                 double timeout_ms,
                                 const Functor& f,
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
    InvokeTimer(struct event_base* evloop, double timeout_ms, const Functor& f, bool periodic);
    void OnTimerTriggered();
    void OnCanceled();

private:
    struct event_base* loop_;
    double timeout_ms_;
    Functor functor_;
    Functor cancel_callback_;
    std::shared_ptr<TimerEventWatcher> timer_;
    bool periodic_;
    std::shared_ptr<InvokeTimer> self_; // Hold myself
};

}
