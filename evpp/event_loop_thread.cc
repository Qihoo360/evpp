#include "evpp/inner_pre.h"

#include "evpp/event_loop.h"
#include "evpp/event_loop_thread.h"

namespace evpp {
EventLoopThread::EventLoopThread()
    : event_loop_(new EventLoop) {
    LOG_INFO << "this=" << this << " EventLoopThread::EventLoopThread loop=" << event_loop_;
}

EventLoopThread::~EventLoopThread() {
    LOG_INFO << "this=" << this << " EventLoopThread::~EventLoopThread";
    assert(IsStopped());
    if (thread_ && thread_->joinable()) {
        LOG_INFO << "this=" << this << " thread=" << thread_ << " joinable";
        try {
            thread_->join();
        } catch (const std::system_error& e) {
            LOG_ERROR << "Caught a system_error:" << e.what() << " code=" << e.code();
        }
    }
}

bool EventLoopThread::Start(bool wait_until_thread_started,
                            const Functor& pre, const Functor& post) {
    status_ = kStarting;
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
    LOG_INFO << "this=" << this << " EventLoopThread::Run loop=" << event_loop_;
    if (name_.empty()) {
        std::ostringstream os;
        os << "thread-" << std::this_thread::get_id();
        name_ = os.str();
    }
    status_ = kRunning;
    if (pre) {
        pre();
    }
    event_loop_->Run();
    if (post) {
        post();
    }
    assert(event_loop_->IsStopped());
    LOG_INFO << "this=" << this << " EventLoopThread stopped";
    status_ = kStopped;
}

void EventLoopThread::Stop(bool wait_thread_exit) {
    assert(status_ == kRunning && IsRunning());
    status_ = kStopping;
    event_loop_->Stop();

    if (wait_thread_exit) {
        while (!IsStopped()) {
            usleep(1);
        }

        if (thread_->joinable()) {
            try {
                thread_->join();
            } catch (const std::system_error& e) {
                LOG_ERROR << "Caught a system_error:" << e.what() << " code=" << e.code();
            }
            thread_.reset();
        }
    }
}

void EventLoopThread::SetName(const std::string& n) {
    name_ = n;
}

const std::string& EventLoopThread::name() const {
    return name_;
}


EventLoop* EventLoopThread::loop() const {
    return event_loop_.get();
}


struct event_base* EventLoopThread::event_base() {
    return loop()->event_base();
}

std::thread::id EventLoopThread::tid() const {
    if (thread_) {
        return thread_->get_id();
    }

    return std::thread::id();
}

bool EventLoopThread::IsRunning() const {
    // Using event_loop_->running() is more exact to query where thread is
    // running or not instead of status_ == kRunning
    //
    // Because in some particular circumstances,
    // when status_==kRunning and event_loop_::running_ == false,
    // the application will broke down
    return event_loop_->running();
}

bool EventLoopThread::IsStopped() const {
    return status_ == kStopped;
}


void EventLoopThread::AfterFork() {
    loop()->AfterFork();
}

}
