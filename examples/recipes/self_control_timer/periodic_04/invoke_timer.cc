#include "invoke_timer.h"
#include "event_watcher.h"

#include <thread>
#include <iostream>

namespace recipes {

InvokeTimer::InvokeTimer(struct event_base* evloop, double timeout_ms, const Functor& f, bool periodic)
    : loop_(evloop), timeout_ms_(timeout_ms), functor_(f), periodic_(periodic) {
    std::cout << "InvokeTimer::InvokeTimer tid=" << std::this_thread::get_id() << " this=" << this << std::endl;
}

InvokeTimerPtr InvokeTimer::Create(struct event_base* evloop, double timeout_ms, const Functor& f, bool periodic) {
    InvokeTimerPtr it(new InvokeTimer(evloop, timeout_ms, f, periodic));
    it->self_ = it;
    return it;
}

InvokeTimer::~InvokeTimer() {
    std::cout << "InvokeTimer::~InvokeTimer tid=" << std::this_thread::get_id() << " this=" << this << std::endl;
}

void InvokeTimer::Start() {
    std::cout << "InvokeTimer::Start tid=" << std::this_thread::get_id() << " this=" << this << " refcount=" << self_.use_count() << std::endl;
    timer_.reset(new TimerEventWatcher(loop_, std::bind(&InvokeTimer::OnTimerTriggered, shared_from_this()), timeout_ms_));
    timer_->SetCancelCallback(std::bind(&InvokeTimer::OnCanceled, shared_from_this()));
    timer_->Init();
    timer_->AsyncWait();
    std::cout << "InvokeTimer::Start(AsyncWait) tid=" << std::this_thread::get_id() << " timer=" << timer_.get() << " this=" << this << " refcount=" << self_.use_count() << " periodic=" << periodic_ << " timeout(ms)=" << timeout_ms_ << std::endl;
}

void InvokeTimer::Cancel() {
    if (timer_) {
        timer_->Cancel();
    }
}

void InvokeTimer::OnTimerTriggered() {
    std::cout << "InvokeTimer::OnTimerTriggered tid=" << std::this_thread::get_id() << " this=" << this << " use_count=" << self_.use_count() << std::endl;
    functor_();

    if (periodic_) {
        timer_->AsyncWait();
    } else {
        functor_ = Functor();
        cancel_callback_ = Functor();
        timer_.reset();
        self_.reset();
    }
}

void InvokeTimer::OnCanceled() {
    std::cout << "InvokeTimer::OnCanceled tid=" << std::this_thread::get_id() << " this=" << this << " use_count=" << self_.use_count() << std::endl;
    periodic_ = false;
    if (cancel_callback_) {
        cancel_callback_();
        cancel_callback_ = Functor();
    }
    functor_ = Functor();
    timer_.reset();
    self_.reset();
}

}
