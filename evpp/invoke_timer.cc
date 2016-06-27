#include "evpp/inner_pre.h"

#include "evpp/event_loop.h"
#include "evpp/libevent_watcher.h"

namespace evpp {

    InvokeTimer::InvokeTimer(EventLoop* evloop, Duration timeout, const Functor& f) 
        : loop_(evloop), timeout_(timeout), functor_(f), timer_(NULL) {
        LOG_INFO << "InvokeTimer::InvokeTimer tid=" << std::this_thread::get_id() << " this=" << this;
    }

    std::shared_ptr<InvokeTimer> InvokeTimer::Create(EventLoop* evloop, Duration timeout, const Functor& f) {
        std::shared_ptr<InvokeTimer> it(new InvokeTimer(evloop, timeout, f));
        it->self_ = it;
        return it;
    }

    InvokeTimer::~InvokeTimer() {
        LOG_INFO << "InvokeTimer::~InvokeTimer tid=" << std::this_thread::get_id() << " this=" << this;
        if (timer_) {
            delete timer_;
            timer_ = NULL;
        }
    }

    void InvokeTimer::Start() {
        LOG_INFO << "InvokeTimer::Start tid=" << std::this_thread::get_id() << " this=" << this << " refcount=" << self_.use_count();
        loop_->RunInLoop(std::bind(&InvokeTimer::AsyncWait, this, timeout_));
    }

    void InvokeTimer::Cancel() {
        if (timer_) {
            loop_->RunInLoop(std::bind(&TimerEventWatcher::Cancel, timer_));
        }
    }

    void InvokeTimer::AsyncWait(Duration timeout) {
        LOG_INFO << "InvokeTimer::AsyncWait tid=" << std::this_thread::get_id() << " this=" << this << " refcount=" << self_.use_count();
        timer_ = new TimerEventWatcher(loop_, std::bind(&InvokeTimer::OnTimeout, this), timeout_);
        timer_->set_cancel_callback(std::bind(&InvokeTimer::OnCanceled, this));
        timer_->Init();
        timer_->AsyncWait();
    }

    void InvokeTimer::OnTimeout() {
        LOG_INFO << "InvokeTimer::OnTimeout tid=" << std::this_thread::get_id() << " this=" << this;
        functor_();
        self_.reset();
    }

    void InvokeTimer::OnCanceled() {
        LOG_INFO << "InvokeTimer::OnCanceled tid=" << std::this_thread::get_id() << " this=" << this;
        self_.reset();
    }
}