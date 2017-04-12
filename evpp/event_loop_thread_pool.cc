#include "evpp/inner_pre.h"
#include "evpp/event_loop_thread_pool.h"
#include "evpp/event_loop.h"

namespace evpp {

EventLoopThreadPool::EventLoopThreadPool(EventLoop* base_loop, uint32_t thread_number)
    : base_loop_(base_loop),
      thread_num_(thread_number) {}

EventLoopThreadPool::~EventLoopThreadPool() {
    assert(thread_num_ == threads_.size());

    for (uint32_t i = 0; i < thread_num_; i++) {
        assert(threads_[i]->IsStopped());
    }

    threads_.clear();
}

bool EventLoopThreadPool::Start(bool wait_until_thread_started) {
    assert(!started_.load());
    if (started_.load()) {
        return true;
    }

    if (thread_num_ == 0) {
        started_.store(true);
        return true;
    }

    std::shared_ptr<std::atomic<uint32_t>> count(new std::atomic<uint32_t>(0));
    for (uint32_t i = 0; i < thread_num_; ++i) {
        auto fn = [this, count]() {
            LOG_INFO << "this=" << this << " a working thread started tid=" << std::this_thread::get_id();
            this->OnThreadStarted(count->fetch_add(1) + 1);
        };
        EventLoopThreadPtr t(new EventLoopThread());
        if (!t->Start(wait_until_thread_started, fn)) {
            //FIXME error process
            LOG_ERROR << "start thread failed!";
            return false;
        }

        std::stringstream ss;
        ss << "EventLoopThreadPool-thread-" << i << "th";
        t->SetName(ss.str());
        threads_.push_back(t);
    }

    // when all the working thread have started,
    // started_ will be stored with true in method OnThreadStarted

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

    started_.store(false);
}

bool EventLoopThreadPool::IsRunning() const {
    if (!started_.load()) {
        return false;
    }

    for (uint32_t i = 0; i < thread_num_; ++i) {
        const EventLoopThreadPtr& t = threads_[i];

        if (!t->IsRunning()) {
            return false;
        }
    }

    return started_.load();
}

bool EventLoopThreadPool::IsStopped() const {
    if (thread_num_ == 0) {
        return !started_.load();
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

    if (started_.load() && !threads_.empty()) {
        // No need to lock here
        int64_t next = next_.fetch_add(1);
        next = next % threads_.size();
        loop = (threads_[next])->loop();
    }

    return loop;
}

EventLoop* EventLoopThreadPool::GetNextLoopWithHash(uint64_t hash) {
    EventLoop* loop = base_loop_;

    if (started_.load() && !threads_.empty()) {
        uint64_t next = hash % threads_.size();
        loop = (threads_[next])->loop();
    }

    return loop;
}

uint32_t EventLoopThreadPool::thread_num() const {
    return thread_num_;
}

void EventLoopThreadPool::OnThreadStarted(uint32_t count) {
    LOG_INFO << "this=" << this << " tid=" << std::this_thread::get_id() << " count=" << count << " started.";
    if (count == thread_num_) {
        LOG_INFO << "this=" << this << " thread pool totally started.";
        started_.store(true);
    }
}

}
