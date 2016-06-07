#include "evpp/inner_pre.h"

#include "evpp/event_loop.h"
#include "evpp/event_loop_thread.h"

namespace evpp {
    class EventLoopThread::Impl {
    public:
        Impl();
        ~Impl();

        bool Start(bool wait_until_thread_started, const Functor& pre, const Functor& post);
        void Stop(bool wait_thread_exit);

        EventLoop* event_loop() const { return event_loop_.get(); }

        bool IsRunning() const;
        bool IsStopped() const;

        void SetName(const std::string& n);
        const std::string& name() const;

        std::thread::id tid() const;

    private:
        void Run(const Functor& pre, const Functor& post);

    private:
        xstd::shared_ptr<EventLoop> event_loop_;
        xstd::shared_ptr<std::thread> thread_;
        enum State {
            kRunning = 1,
            kStopping = 2,
            kStopped = 3,
        };
        State state_;

        //TODO add thread name here
        // std::string name_;
    };


    EventLoopThread::Impl::Impl()
        : event_loop_(new EventLoop)
        , state_(kStopped) {}

    EventLoopThread::Impl::~Impl() {}

    bool EventLoopThread::Impl::Start(bool wait_until_thread_started,
        const Functor& pre, const Functor& post) {
        thread_.reset(new std::thread(
            xstd::bind(&Impl::Run, this, pre, post)));
        if (!wait_until_thread_started) {
            return true;
        }

        // waiting until the thread started
        for (;;) {
            if (IsRunning()) {
                break;
            }

            usleep(1);
        }

        return true;
    }

    void EventLoopThread::Impl::Run(const Functor& pre, const Functor& post) {
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

    void EventLoopThread::Impl::Stop(bool wait_thread_exit) {
        state_ = kStopping;
        event_loop_->Stop();

        if (wait_thread_exit) {
            thread_->join();
        }
    }

    void EventLoopThread::Impl::SetName(const std::string& n) {
        //TODO
    }

    const std::string& EventLoopThread::Impl::name() const {
        //TODO
        const static std::string __s_empty;
        return __s_empty;

    }

    std::thread::id EventLoopThread::Impl::tid() const {
        if (thread_) {
            return thread_->get_id();
        }
        return std::thread::id();
    }

    bool EventLoopThread::Impl::IsRunning() const {
        return thread_ && state_ == kRunning;
    }

    bool EventLoopThread::Impl::IsStopped() const {
        return state_ == kStopped;
    }

    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    EventLoopThread::EventLoopThread()
        : impl_(new Impl()) {}

    EventLoopThread::~EventLoopThread() {}

    bool EventLoopThread::Start(bool wait_until_thread_started, const Functor& pre, const Functor& post) {
        return impl_->Start(wait_until_thread_started, pre, post);
    }

    void EventLoopThread::Stop(bool wait_thread_exit) {
        impl_->Stop(wait_thread_exit);
    }

    EventLoop* EventLoopThread::event_loop() const {
        return impl_->event_loop();
    }

    struct event_base * EventLoopThread::event_base() {
        return event_loop()->event_base();
    }

    bool EventLoopThread::IsRunning() const {
        return impl_->IsRunning();
    }

    bool EventLoopThread::IsStopped() const {
        return impl_->IsStopped();
    }

    void EventLoopThread::SetName(const std::string& n) {
        return impl_->SetName(n);
    }

    const std::string& EventLoopThread::name() const {
        return impl_->name();
    }

    std::thread::id EventLoopThread::tid() const {
        return impl_->tid();
    }
}
