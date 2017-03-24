#include "evpp/inner_pre.h"

#include "evpp/event_loop.h"
#include "evpp/libevent_watcher.h"

namespace evpp {

InvokeTimer::InvokeTimer(EventLoop* evloop, Duration timeout, const Functor& f, bool periodic)
    : loop_(evloop), timeout_(timeout), functor_(f), periodic_(periodic) {
    LOG_INFO << "InvokeTimer::InvokeTimer tid=" << std::this_thread::get_id() << " this=" << this;
}

InvokeTimer::InvokeTimer(EventLoop* evloop, Duration timeout, Functor&& f, bool periodic)
    : loop_(evloop), timeout_(timeout), functor_(std::move(f)), periodic_(periodic) {
    LOG_INFO << "InvokeTimer::InvokeTimer tid=" << std::this_thread::get_id() << " this=" << this;
}

std::shared_ptr<InvokeTimer> InvokeTimer::Create(EventLoop* evloop, Duration timeout, const Functor& f, bool periodic) {
    std::shared_ptr<InvokeTimer> it(new InvokeTimer(evloop, timeout, f, periodic));
    it->self_ = it;
    return it;
}

std::shared_ptr<InvokeTimer> InvokeTimer::Create(EventLoop* evloop, Duration timeout, Functor&& f, bool periodic) {
    std::shared_ptr<InvokeTimer> it(new InvokeTimer(evloop, timeout, std::move(f), periodic));
    it->self_ = it;
    return it;
}

InvokeTimer::~InvokeTimer() {
    LOG_INFO << "InvokeTimer::~InvokeTimer tid=" << std::this_thread::get_id() << " this=" << this;
}

void InvokeTimer::Start() {
    LOG_INFO << "InvokeTimer::Start tid=" << std::this_thread::get_id() << " this=" << this << " refcount=" << self_.use_count();
    auto f = [this]() {
        timer_.reset(new TimerEventWatcher(loop_, std::bind(&InvokeTimer::OnTimerTriggered, shared_from_this()), timeout_));
        timer_->SetCancelCallback(std::bind(&InvokeTimer::OnCanceled, shared_from_this()));
        timer_->Init();
        timer_->AsyncWait();
        LOG_INFO << "InvokeTimer::Start(AsyncWait) tid=" << std::this_thread::get_id() << " timer=" << timer_.get() << " this=" << this << " refcount=" << self_.use_count() << " periodic=" << periodic_ << " timeout(ms)=" << timeout_.Milliseconds();
    };
    loop_->RunInLoop(std::move(f));
}

void InvokeTimer::Cancel() {
    if (timer_) {
        loop_->QueueInLoop(std::bind(&TimerEventWatcher::Cancel, timer_));
    }
}

void InvokeTimer::OnTimerTriggered() {
    LOG_INFO << "InvokeTimer::OnTimerTriggered tid=" << std::this_thread::get_id() << " this=" << this << " use_count=" << self_.use_count();
    functor_();

    if (periodic_) {
        timer_->AsyncWait();
    } else {
        functor_ = Functor();
        timer_.reset();
        self_.reset();
    }
}

void InvokeTimer::OnCanceled() {
    LOG_INFO << "InvokeTimer::OnCanceled tid=" << std::this_thread::get_id() << " this=" << this << " use_count=" << self_.use_count();
    periodic_ = false;
    if (cancel_callback_) {
        cancel_callback_();
        cancel_callback_ = Functor();
    }
    timer_.reset();
    self_.reset();
}
}
