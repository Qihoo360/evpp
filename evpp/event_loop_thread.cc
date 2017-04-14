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
    Join();
}

bool EventLoopThread::Start(bool wait_thread_started, Functor pre, Functor post) {
    status_ = kStarting;

    assert(thread_.get() == nullptr);
    thread_.reset(new std::thread(std::bind(&EventLoopThread::Run, this, pre, post)));

    if (wait_thread_started) {
        while (status_ < kRunning) {
            usleep(1);
        }
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


    LOG_INFO << "this=" << this << " execute pre functor.";
    auto fn = [this, pre]() {
        status_ = kRunning;
        if (pre) {
            auto rc = pre();
            if (rc != kOK) {
                event_loop_->Stop();
            }
        }
    };
    event_loop_->QueueInLoop(std::move(fn));
    event_loop_->Run();

    LOG_INFO << "this=" << this << " execute post functor.";
    if (post) {
        post();
    }

    assert(event_loop_->IsStopped());
    LOG_INFO << "this=" << this << " EventLoopThread stopped";
    status_ = kStopped;
}

void EventLoopThread::Stop(bool wait_thread_exit) {
    LOG_INFO << "this=" << this << " loop=" << event_loop_ << " EventLoopThread::Stop wait_thread_exit=" << wait_thread_exit;
    assert(status_ == kRunning && IsRunning());
    status_ = kStopping;
    event_loop_->Stop();

    if (wait_thread_exit) {
        while (!IsStopped()) {
            usleep(1);
        }

        LOG_INFO << "this=" << this << " thread stopped.";
        Join();
        LOG_INFO << "this=" << this << " thread totally stopped.";
    }
    LOG_INFO << "this=" << this << " loop=" << event_loop_ << " EventLoopThread::Stop";
}

void EventLoopThread::Join() {
    // To avoid multi other threads call Join simultaneously
    std::lock_guard<std::mutex> guard(mutex_);
    if (thread_ && thread_->joinable()) {
        LOG_INFO << "this=" << this << " thread=" << thread_ << " joinable";
        try {
            thread_->join();
        } catch (const std::system_error& e) {
            LOG_ERROR << "Caught a system_error:" << e.what() << " code=" << e.code();
        }
        thread_.reset();
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
