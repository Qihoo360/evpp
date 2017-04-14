#include "invoke_timer.h"
#include "event_watcher.h"

#include <thread>
#include <iostream>

namespace recipes {

InvokeTimer::InvokeTimer(struct event_base* evloop, double timeout_ms, const Functor& f)
    : loop_(evloop), timeout_ms_(timeout_ms), functor_(f) {
    std::cout << "InvokeTimer::InvokeTimer tid=" << std::this_thread::get_id() << " this=" << this << std::endl;
}

InvokeTimer* InvokeTimer::Create(struct event_base* evloop, double timeout_ms, const Functor& f) {
    return new InvokeTimer(evloop, timeout_ms, f);
}

InvokeTimer::~InvokeTimer() {
    std::cout << "InvokeTimer::~InvokeTimer tid=" << std::this_thread::get_id() << " this=" << this << std::endl;
}

void InvokeTimer::Start() {
    std::cout << "InvokeTimer::Start tid=" << std::this_thread::get_id() << " this=" << this << std::endl;
    timer_.reset(new TimerEventWatcher(loop_, std::bind(&InvokeTimer::OnTimerTriggered, this), timeout_ms_));
    timer_->Init();
    timer_->AsyncWait();
    std::cout << "InvokeTimer::Start(AsyncWait) tid=" << std::this_thread::get_id() << " timer=" << timer_.get() << " this=" << this << " timeout(ms)=" << timeout_ms_ << std::endl;
}

void InvokeTimer::OnTimerTriggered() {
    std::cout << "InvokeTimer::OnTimerTriggered tid=" << std::this_thread::get_id() << " this=" << this << std::endl;
    functor_();
    functor_ = Functor();
    delete this;
}

}
