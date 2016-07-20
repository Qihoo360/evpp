#include "evpp/inner_pre.h"

#include "evpp/event_loop.h"
#include "evpp/event_loop_thread.h"

namespace evpp {
EventLoopThread::EventLoopThread()
    : event_loop_(new EventLoop)
    , state_(kStopped) {
}

EventLoopThread::~EventLoopThread() {
    if (thread_ && thread_->joinable()) {
        thread_->join();
    }
}

bool EventLoopThread::Start(bool wait_until_thread_started,
                                  const Functor& pre, const Functor& post) {
    thread_.reset(new std::thread(
                      std::bind(&EventLoopThread::Run, this, pre, post)));

    // waiting until the thread started
    while (wait_until_thread_started) {
        if (IsRunning()) {
            break;
        }

        usleep(1);
    }

    return true;
}

void EventLoopThread::Run(const Functor& pre, const Functor& post) {
    if (name_.empty()) {
        std::ostringstream os;
        os << "thread-" << std::this_thread::get_id();
        name_ = os.str();
    }

    state_ = kRunning;

    if (pre) {
        pre();
    }

    event_loop_->Run();

    if (post) {
        post();
    }

    state_ = kStopped;
}

void EventLoopThread::Stop(bool wait_thread_exit) {
    state_ = kStopping;
    event_loop_->Stop();

    if (wait_thread_exit) {
        thread_->join();
    }
}

void EventLoopThread::SetName(const std::string& n) {
    name_ = n;
}

const std::string& EventLoopThread::name() const {
    return name_;
}


EventLoop* EventLoopThread::event_loop() const {
    return event_loop_.get();
}


struct event_base* EventLoopThread::event_base() {
    return event_loop()->event_base();
}

std::thread::id EventLoopThread::tid() const {
    if (thread_) {
        return thread_->get_id();
    }

    return std::thread::id();
}

bool EventLoopThread::IsRunning() const {
    return thread_ && state_ == kRunning;
}

bool EventLoopThread::IsStopped() const {
    return state_ == kStopped;
}

}
