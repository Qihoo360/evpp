#include "evpp/inner_pre.h"
#include "evpp/event_loop_thread_pool.h"
#include "evpp/event_loop.h"

namespace evpp {

EventLoopThreadPool::EventLoopThreadPool(EventLoop* base_loop, uint32_t thread_number)
    : base_loop_(base_loop),
      thread_num_(thread_number) {
    LOG_INFO << "this=" << this << " EventLoopThreadPool::EventLoopThreadPool thread_num=" << thread_num() << " base loop=" << base_loop_;
}

EventLoopThreadPool::~EventLoopThreadPool() {
    LOG_INFO << "this=" << this << " EventLoopThreadPool::~EventLoopThreadPool thread_num=" << thread_num();
    Join();
    threads_.clear();
}

bool EventLoopThreadPool::Start(bool wait_thread_started) {
    status_.store(kStarting);
    LOG_INFO << "this=" << this << " EventLoopThreadPool::Start thread_num=" << thread_num() << " base loop=" << base_loop_ << " wait_thread_started=" << wait_thread_started;

    if (thread_num_ == 0) {
        status_.store(kRunning);
        return true;
    }

    std::shared_ptr<std::atomic<uint32_t>> started_count(new std::atomic<uint32_t>(0));
    std::shared_ptr<std::atomic<uint32_t>> exited_count(new std::atomic<uint32_t>(0));
    for (uint32_t i = 0; i < thread_num_; ++i) {
        auto prefn = [this, started_count]() {
            LOG_INFO << "this=" << this << " a working thread started tid=" << std::this_thread::get_id();
            this->OnThreadStarted(started_count->fetch_add(1) + 1);
            return EventLoopThread::kOK;
        };

        auto postfn = [this, exited_count]() {
            LOG_INFO << "this=" << this << " a working thread exiting, tid=" << std::this_thread::get_id();
            this->OnThreadExited(exited_count->fetch_add(1) + 1);
            return EventLoopThread::kOK;
        };

        EventLoopThreadPtr t(new EventLoopThread());
        if (!t->Start(wait_thread_started, prefn, postfn)) {
            //FIXME error process
            LOG_ERROR << "start thread failed!";
            return false;
        }

        std::stringstream ss;
        ss << "EventLoopThreadPool-thread-" << i << "th";
        t->set_name(ss.str());
        threads_.push_back(t);
    }

    // when all the working thread have started,
    // status_ will be stored with kRunning in method OnThreadStarted

    if (wait_thread_started) {
        while (!IsRunning()) {
            usleep(1);
        }
        assert(status_.load() == kRunning);
    }

    return true;
}

void EventLoopThreadPool::Stop(bool wait_thread_exit) {
    LOG_INFO << "this=" << this << " EventLoopThreadPool::Stop wait_thread_exit=" << wait_thread_exit;
    Stop(wait_thread_exit, Functor());
}

void EventLoopThreadPool::Stop(Functor on_stopped_cb) {
    LOG_INFO << "this=" << this << " EventLoopThreadPool::Stop";
    Stop(false, on_stopped_cb);
}

void EventLoopThreadPool::Stop(bool wait_thread_exit, Functor on_stopped_cb) {
    status_.store(kStopping);
    
    if (thread_num_ == 0) {
        status_.store(kStopped);
        
        if (on_stopped_cb) {
            LOG_INFO << "this=" << this << " EventLoopThreadPool::Stop calling stopped callback";
            on_stopped_cb();
        }
        return;
    }

    LOG_INFO << "this=" << this << " EventLoopThreadPool::Stop wait_thread_exit=" << wait_thread_exit;
    stopped_cb_ = on_stopped_cb;

    for (auto &t : threads_) {
        t->Stop();
    }

    // when all the working thread have stopped
    // status_ will be stored with kStopped in method OnThreadExited

    auto is_stopped_fn = [this]() {
        for (auto &t : this->threads_) {
            if (!t->IsStopped()) {
                return false;
            }
        }
        return true;
    };

    LOG_INFO << "this=" << this << " EventLoopThreadPool::Stop before promise wait";
    if (thread_num_ > 0 && wait_thread_exit) {
        while (!is_stopped_fn()) {
            usleep(1);
        }
    }
    LOG_INFO << "this=" << this << " EventLoopThreadPool::Stop after promise wait";

    status_.store(kStopped);
}

void EventLoopThreadPool::Join() {
    LOG_INFO << "this=" << this << " EventLoopThreadPool::Join thread_num=" << thread_num();
    for (auto &t : threads_) {
        t->Join();
    }
    threads_.clear();
}

// bool EventLoopThreadPool::IsRunning() const {
//     if (!started_.load()) {
//         return false;
//     }
// 
//     for (auto &t : threads_) {
//         if (!t->IsRunning()) {
//             return false;
//         }
//     }
// 
//     return started_.load();
// }
// 
// bool EventLoopThreadPool::IsStopped() const {
//     if (thread_num_ == 0) {
//         return !started_.load();
//     }
// 
//     for (auto &t : threads_) {
//         if (!t->IsStopped()) {
//             return false;
//         }
//     }
// 
//     return true;
// }

EventLoop* EventLoopThreadPool::GetNextLoop() {
    EventLoop* loop = base_loop_;

    if (IsRunning() && !threads_.empty()) {
        // No need to lock here
        int64_t next = next_.fetch_add(1);
        next = next % threads_.size();
        loop = (threads_[next])->loop();
    }

    return loop;
}

EventLoop* EventLoopThreadPool::GetNextLoopWithHash(uint64_t hash) {
    EventLoop* loop = base_loop_;

    if (IsRunning() && !threads_.empty()) {
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
        status_.store(kRunning);
    }
}

void EventLoopThreadPool::OnThreadExited(uint32_t count) {
    LOG_INFO << "this=" << this << " tid=" << std::this_thread::get_id() << " count=" << count << " exited.";
    if (count == thread_num_) {
        status_.store(kStopped);
        LOG_INFO << "this=" << this << " this is the last thread stopped. Thread pool totally exited.";
        if (stopped_cb_) {
            stopped_cb_();
            stopped_cb_ = Functor();
        }
    }
}

}
