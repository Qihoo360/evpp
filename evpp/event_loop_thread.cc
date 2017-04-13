#include "evpp/inner_pre.h"

#include "evpp/event_loop.h"
#include "evpp/event_loop_thread.h"

namespace evpp {

EventLoopThread::EventLoopThread()
    : event_loop_(new EventLoop)
    , post_task_([]() { return kOK; }) {
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

bool EventLoopThread::Start(bool wait_thread_started, Functor pre, Functor post) {
    status_ = kStarting;

    if (post) {
        post_task_ = std::packaged_task<int()>(std::move(post));
    }

    thread_.reset(new std::thread(std::bind(&EventLoopThread::Run, this, pre)));

    int rc = kOK;
    if (wait_thread_started) {
        auto result = pre_task_promise_.get_future();
        result.wait();
        rc = result.get();
        assert(IsRunning());
    }
    return rc == kOK;
}

void EventLoopThread::Run(const Functor& pre) {
    LOG_INFO << "this=" << this << " EventLoopThread::Run loop=" << event_loop_;
    if (name_.empty()) {
        std::ostringstream os;
        os << "thread-" << std::this_thread::get_id();
        name_ = os.str();
    }


    LOG_INFO << "this=" << this << " execute pre functor.";
    auto fn = [this, pre]() {
        if (pre) {
            pre_task_promise_.set_value(pre());
        } else {
            pre_task_promise_.set_value(kOK);
        }
    };
    event_loop_->QueueInLoop(std::move(fn));

    status_ = kRunning;
    event_loop_->Run();
    status_ = kStopped;

    LOG_INFO << "this=" << this << " execute post functor.";
    if (post_task_.valid()) {
        post_task_();
    }

    assert(event_loop_->IsStopped());
    LOG_INFO << "this=" << this << " EventLoopThread stopped";
}

void EventLoopThread::Stop(bool wait_thread_exit) {
    assert(status_ == kRunning && IsRunning());
    status_ = kStopping;
    event_loop_->Stop();

    if (wait_thread_exit) {
        auto result = post_task_.get_future();
        result.wait();

        LOG_INFO << "this=" << this << " post function execution result is " << result.get();

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

void EventLoopThread::set_name(const std::string& n) {
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
    return event_loop_->IsRunning();
}

bool EventLoopThread::IsStopped() const {
    return status_ == kStopped;
}


void EventLoopThread::AfterFork() {
    loop()->AfterFork();
}

}
