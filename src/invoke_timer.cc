#include "evpp/inner_pre.h"

#include "evpp/event_loop.h"
#include "evpp/libevent_watcher.h"

namespace evpp {

    InvokeTimer::InvokeTimer(EventLoop* evloop, Duration timeout, const Functor& f) 
        : loop_(evloop), timeout_(timeout), functor_(f), timer_(NULL) {

    }

    std::shared_ptr<InvokeTimer> InvokeTimer::Create(EventLoop* evloop, Duration timeout, const Functor& f) {
        std::shared_ptr<InvokeTimer> it(new InvokeTimer(evloop, timeout, f));
        return it;
    }

    InvokeTimer::~InvokeTimer() {
        LOG_INFO << "InvokeTimer::~InvokeTimer tid=" << std::this_thread::get_id() << " this=" << this;
        DeleteTimerEventWatcher();
    }

    void InvokeTimer::Start() {
        std::shared_ptr<InvokeTimer> ref = shared_from_this(); // reference count +1
        LOG_INFO << "InvokeTimer::Start tid=" << std::this_thread::get_id() << " this=" << this << " refcount=" << ref.use_count();
        loop_->RunInLoop(std::bind(&InvokeTimer::AsyncWait, ref, timeout_));
    }

    void InvokeTimer::Cancel() {
        if (timer_) {
            loop_->RunInLoop(std::tr1::bind(&TimerEventWatcher::Cancel, timer_));
        }
    }

    void InvokeTimer::AsyncWait(Duration timeout) {
        std::shared_ptr<InvokeTimer> ref = shared_from_this(); // ref_count++
        LOG_INFO << "InvokeTimer::AsyncWait tid=" << std::this_thread::get_id() << " this=" << this << " refcount=" << ref.use_count();
        timer_ = new TimerEventWatcher(loop_, std::bind(&InvokeTimer::OnTimeout, this), timeout_);
        //timer_->set_cancel_callback(std::bind(&InvokeTimer::OnCanceled, ref)); // ref_count++
        timer_->Init();
        timer_->AsyncWait();
    }

    void InvokeTimer::OnTimeout() {
        LOG_INFO << "InvokeTimer::OnTimeout tid=" << std::this_thread::get_id() << " this=" << this;
        functor_();
        //DeleteTimerEventWatcher();
    }

    void InvokeTimer::OnCanceled() {
        LOG_INFO << "InvokeTimer::OnCanceled tid=" << std::this_thread::get_id() << " this=" << this;
        //DeleteTimerEventWatcher();
    }

    void InvokeTimer::DeleteTimerEventWatcher() {
        if (timer_) {
            delete timer_;
            timer_ = NULL;
        }
    }

}