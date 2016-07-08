#include "evpp/inner_pre.h"
#include "evpp/event_loop_thread_pool.h"

#include <atomic>

namespace evpp {
    class EventLoopThreadPool::Impl {
    public:
        Impl(EventLoop* base_loop, int thread_number);
        ~Impl();
        bool Start(bool wait_until_thread_started);
        void Stop(bool wait_thread_exit);

        EventLoop* GetNextLoop();
        EventLoop* GetNextLoopWithHash(uint64_t hash);

        bool IsRunning() const;
        bool IsStopped() const;
        int thread_num() const { return thread_num_; }
    private:
        EventLoop* base_loop_;
        bool started_;
        int thread_num_;
        std::atomic<int> next_;

        typedef std::shared_ptr<EventLoopThread> EventLoopThreadPtr;
        std::vector<EventLoopThreadPtr> threads_;
    };

    EventLoopThreadPool::Impl::Impl(EventLoop* base_loop, int thread_number)
        : base_loop_(base_loop),
        started_(false),
        thread_num_(thread_number),
        next_(0) {}

    EventLoopThreadPool::Impl::~Impl() {
        assert(thread_num_ == (int)threads_.size());
        for (int i = 0; i < thread_num_; i++) {
            assert(threads_[i]->IsStopped());
        }
        threads_.clear();
    }

    bool EventLoopThreadPool::Impl::Start(bool wait_until_thread_started) {
        assert(!started_);
        if (started_) {
            return true;
        }

        for (int i = 0; i < thread_num_; ++i) {
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

    void EventLoopThreadPool::Impl::Stop(bool wait_thread_exit) {
        for (int i = 0; i < thread_num_; ++i) {
            EventLoopThreadPtr& t = threads_[i];
            t->Stop(wait_thread_exit);
        }

        if (wait_thread_exit) {
            if (!IsStopped()) {
                usleep(1);
            }
        }
    }

    bool EventLoopThreadPool::Impl::IsRunning() const {
        for (int i = 0; i < thread_num_; ++i) {
            const EventLoopThreadPtr& t = threads_[i];
            if (!t->IsRunning()) {
                return false;
            }
        }

        return true;
    }

    bool EventLoopThreadPool::Impl::IsStopped() const {
        for (int i = 0; i < thread_num_; ++i) {
            const EventLoopThreadPtr& t = threads_[i];
            if (!t->IsStopped()) {
                return false;
            }
        }

        return true;
    }

    EventLoop* EventLoopThreadPool::Impl::GetNextLoop() {
        EventLoop* loop = base_loop_;
        if (!threads_.empty()) {
            // No need to lock here
            int next = next_.fetch_add(1);
            next = next % threads_.size();
            loop = (threads_[next])->event_loop();
        }
        return loop;
    }

    EventLoop* EventLoopThreadPool::Impl::GetNextLoopWithHash(uint64_t hash) {
        EventLoop* loop = base_loop_;

        if (!threads_.empty()) {
            unsigned int next = hash % threads_.size();
            loop = (threads_[next])->event_loop();
        }
        return loop;
    }


    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    EventLoopThreadPool::EventLoopThreadPool(EventLoop* base_loop, int thread_number)
        : impl_(new Impl(base_loop, thread_number)) {}

    EventLoopThreadPool::~EventLoopThreadPool() {}

    bool EventLoopThreadPool::Start(bool wait_until_thread_started) {
        return impl_->Start(wait_until_thread_started);
    }

    void EventLoopThreadPool::Stop(bool wait_thread_exit) {
        impl_->Stop(wait_thread_exit);
    }

    EventLoop* EventLoopThreadPool::GetNextLoop() {
        return impl_->GetNextLoop();
    }

    EventLoop* EventLoopThreadPool::GetNextLoopWithHash(uint64_t hash) {
        return impl_->GetNextLoopWithHash(hash);
    }

    bool EventLoopThreadPool::IsRunning() const {
        return impl_->IsRunning();
    }

    bool EventLoopThreadPool::IsStopped() const {
        return impl_->IsStopped();
    }


    int EventLoopThreadPool::thread_num() const {
        return impl_->thread_num();
    }
}
