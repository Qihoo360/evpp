#include "evpp/inner_pre.h"
#include "evpp/event_loop_thread_pool.h"
#include "evpp/event_loop.h"

namespace evpp {

EventLoopThreadPool::EventLoopThreadPool(EventLoop* base_loop, uint32_t thread_number)
    : base_loop_(base_loop),
      started_(false),
      thread_num_(thread_number),
      next_(0) {}

EventLoopThreadPool::~EventLoopThreadPool() {
    assert(thread_num_ == threads_.size());

    for (uint32_t i = 0; i < thread_num_; i++) {
        assert(threads_[i]->IsStopped());
    }

    threads_.clear();
}

bool EventLoopThreadPool::Start(bool wait_until_thread_started) {
    assert(!started_);

    if (started_) {
        return true;
    }

    for (uint32_t i = 0; i < thread_num_; ++i) {
        std::stringstream ss;
        ss << "EventLoopThreadPool-thread-" << i << "th";
        EventLoopThreadPtr t(new EventLoopThread());

        if (!t->Start(wait_until_thread_started)) {
            //FIXME error process
            LOG_ERROR << "start thread failed!";
            return false;
        }

        t->SetName(ss.str());
        threads_.push_back(t);
    }

    started_ = true;
    return true;
}

void EventLoopThreadPool::Stop(bool wait_thread_exit) {
    for (uint32_t i = 0; i < thread_num_; ++i) {
        EventLoopThreadPtr& t = threads_[i];
        t->Stop(wait_thread_exit);
    }

    if (thread_num_ > 0 && wait_thread_exit) {
        while (!IsStopped()) {
            usleep(1);
        }
    }

    started_ = false;
}

bool EventLoopThreadPool::IsRunning() const {
    if (!started_) {
        return false;
    }

    for (uint32_t i = 0; i < thread_num_; ++i) {
        const EventLoopThreadPtr& t = threads_[i];

        if (!t->IsRunning()) {
            return false;
        }
    }

    return started_;
}

bool EventLoopThreadPool::IsStopped() const {
    if (thread_num_ == 0) {
        return !started_;
    }

    for (uint32_t i = 0; i < thread_num_; ++i) {
        const EventLoopThreadPtr& t = threads_[i];

        if (!t->IsStopped()) {
            return false;
        }
    }

    return true;
}

EventLoop* EventLoopThreadPool::GetNextLoop() {
    EventLoop* loop = base_loop_;

    if (!threads_.empty()) {
        // No need to lock here
        int64_t next = next_.fetch_add(1);
        next = next % threads_.size();
        loop = (threads_[next])->event_loop();
    }

    return loop;
}

EventLoop* EventLoopThreadPool::GetNextLoopWithHash(uint64_t hash) {
    EventLoop* loop = base_loop_;

    if (!threads_.empty()) {
        uint64_t next = hash % threads_.size();
        loop = (threads_[next])->event_loop();
    }

    return loop;
}

uint32_t EventLoopThreadPool::thread_num() const {
    return thread_num_;
}

}
