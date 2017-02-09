#include "evpp/inner_pre.h"

#include "evpp/event_loop.h"
#include "evpp/event_loop_thread.h"

namespace evpp {
EventLoopThread::EventLoopThread()
    : event_loop_(new EventLoop)
    , status_(kStopped) {
}

EventLoopThread::~EventLoopThread() {
    if (thread_ && thread_->joinable()) {
        try {
            thread_->join();
        } catch (const std::system_error& e) {
            LOG_ERROR << "Caught a system_error:" << e.what();
        }
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
    status_ = kRunning;
    if (pre) {
        pre();
    }
    event_loop_->Run();
    if (post) {
        post();
    }
    event_loop_.reset(); // 就在当前线程中释放所有资源
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
                LOG_ERROR << "Caught a system_error:" << e.what();
            }
        }
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
    // 使 event_loop_->running() 这种判断方式更准确，而不是 status_==kRunning 。
    // 这是因为：在极端情况下， status_==kRunning，但 event_loop_::running_ == false，
    //          这种情况下某些依赖EventLoop运行的代码将发生异常。
    return event_loop_->running();
}

bool EventLoopThread::IsStopped() const {
    return status_ == kStopped;
}


void EventLoopThread::AfterFork() {
    event_loop()->AfterFork();
}

}
