#include "evpp/inner_pre.h"

#include "evpp/event_loop.h"
#include "evpp/libevent_watcher.h"

namespace evpp {

InvokeTimer::InvokeTimer(EventLoop* evloop, Duration timeout, const Functor& f, bool periodic)
    : loop_(evloop), timeout_(timeout), functor_(f), timer_(nullptr), periodic_(periodic) {
    LOG_INFO << "InvokeTimer::InvokeTimer tid=" << std::this_thread::get_id() << " this=" << this;
}

std::shared_ptr<InvokeTimer> InvokeTimer::Create(EventLoop* evloop, Duration timeout, const Functor& f, bool periodic) {
    std::shared_ptr<InvokeTimer> it(new InvokeTimer(evloop, timeout, f, periodic));
    it->self_ = it;
    return it;
}

InvokeTimer::~InvokeTimer() {
    LOG_INFO << "InvokeTimer::~InvokeTimer tid=" << std::this_thread::get_id() << " this=" << this;

    if (timer_) {
        delete timer_;
        timer_ = nullptr;
    }
}

void InvokeTimer::Start() {
    LOG_INFO << "InvokeTimer::Start tid=" << std::this_thread::get_id() << " this=" << this << " refcount=" << self_.use_count();
    loop_->RunInLoop(std::bind(&InvokeTimer::AsyncWait, this));
}

void InvokeTimer::Cancel() {
    if (timer_) {
        loop_->RunInLoop(std::bind(&TimerEventWatcher::Cancel, timer_));
    }
}

void InvokeTimer::AsyncWait() {
    //LOG_INFO << "InvokeTimer::AsyncWait tid=" << std::this_thread::get_id() << " this=" << this << " refcount=" << self_.use_count();
    timer_ = new TimerEventWatcher(loop_, std::bind(&InvokeTimer::OnTimerTriggered, this), timeout_);
    timer_->SetCancelCallback(std::bind(&InvokeTimer::OnCanceled, this));
    timer_->Init();
    timer_->AsyncWait();
}

void InvokeTimer::OnTimerTriggered() {
    //LOG_INFO << "InvokeTimer::OnTimerTriggered tid=" << std::this_thread::get_id() << " this=" << this;
    functor_();

    if (periodic_) {
        timer_->AsyncWait();
    } else {
        self_.reset();
    }
}

void InvokeTimer::OnCanceled() {
    //LOG_INFO << "InvokeTimer::OnCanceled tid=" << std::this_thread::get_id() << " this=" << this;
    self_.reset();
}
}
