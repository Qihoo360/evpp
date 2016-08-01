#include "evpp/inner_pre.h"

#include "evpp/libevent_headers.h"
#include "evpp/libevent_watcher.h"
#include "evpp/event_loop.h"
#include "evpp/invoke_timer.h"

namespace evpp {
EventLoop::EventLoop() : create_evbase_myself_(true), pending_functor_count_(0) {
#if LIBEVENT_VERSION_NUMBER >= 0x02001500
    struct event_config* cfg = event_config_new();
    if (cfg) {
        // Does not cache time to get a preciser timer
        event_config_set_flag(cfg, EVENT_BASE_FLAG_NO_CACHE_TIME);
        evbase_ = event_base_new_with_config(cfg);
        event_config_free(cfg);
    }
#else
    evbase_ = event_base_new();
#endif
    Init();
}

EventLoop::EventLoop(struct event_base* base)
    : evbase_(base), create_evbase_myself_(false) {
    Init();

    watcher_.reset(new PipeEventWatcher(this, std::bind(&EventLoop::DoPendingFunctors, this)));
    int rc = watcher_->Init();
    assert(rc);
    rc = rc && watcher_->AsyncWait();
    assert(rc);
    if (!rc) {
        LOG_ERROR << "PipeEventWatcher init failed.";
    }
}

EventLoop::~EventLoop() {
    if (!create_evbase_myself_) {
        assert(watcher_);
        watcher_.reset();
    }
    assert(!watcher_.get());

    if (evbase_ != NULL && create_evbase_myself_) {
        event_base_free(evbase_);
        evbase_ = NULL;
    }
}

void EventLoop::Init() {
    running_ = false;
    tid_ = std::this_thread::get_id(); // The default thread id
    calling_pending_functors_ = false;
}

void EventLoop::Run() {
    watcher_.reset(new PipeEventWatcher(this, std::bind(&EventLoop::DoPendingFunctors, this)));
    int rc = watcher_->Init();
    assert(rc);
    rc = rc && watcher_->AsyncWait();
    assert(rc);
    if (!rc) {
        LOG_ERROR << "PipeEventWatcher init failed.";
    }

    tid_ = std::this_thread::get_id(); // The actual thread id

    // 所有的事情都准备好之后，才置标记为true
    running_ = true;
    rc = event_base_dispatch(evbase_);

    if (rc == 1) {
        LOG_ERROR << "event_base_dispatch error: no event registered";
    } else if (rc == -1) {
        int serrno = errno;
        LOG_ERROR << "event_base_dispatch error " << serrno << " " << strerror(serrno);
    }

    watcher_.reset(); // 确保在同一个线程构造、初始化和析构
    running_ = false;
    LOG_TRACE << "EventLoop stopped, tid: " << std::this_thread::get_id();
}

bool EventLoop::IsInLoopThread() const {
    std::thread::id cur_tid = std::this_thread::get_id();
    return cur_tid == tid_;
}

void EventLoop::Stop() {
    assert(running_);
    RunInLoop(std::bind(&EventLoop::StopInLoop, this));
}

void EventLoop::StopInLoop() {
    LOG_TRACE << "EventLoop is stopping now, tid=" << std::this_thread::get_id();
    assert(running_);
    for (;;) {
        DoPendingFunctors();

        std::lock_guard<std::mutex> lock(mutex_);

        if (pending_functors_.empty()) {
            break;
        }
    }

    timeval tv = Duration(0.5).TimeVal(); // 0.5 second
    event_base_loopexit(evbase_, &tv);
    running_ = false;
}

void EventLoop::AfterFork() {
    int rc = event_reinit(evbase_);
    assert(rc == 0);

    if (rc != 0) {
        fprintf(stderr, "event_reinit failed!\n");
        abort();
    }
}

InvokeTimerPtr EventLoop::RunAfter(double delay_ms, const Functor& f) {
    return RunAfter(Duration(delay_ms / 1000.0), f);
}

InvokeTimerPtr EventLoop::RunAfter(Duration delay, const Functor& f) {
    std::shared_ptr<InvokeTimer> t = InvokeTimer::Create(this, delay, f, false);
    t->Start();
    return t;
}

evpp::InvokeTimerPtr EventLoop::RunEvery(Duration interval, const Functor& f) {
    std::shared_ptr<InvokeTimer> t = InvokeTimer::Create(this, interval, f, true);
    t->Start();
    return t;
}

void EventLoop::RunInLoop(const Functor& functor) {
    if (IsInLoopThread()) {
        functor();
    } else {
        QueueInLoop(functor);
    }
}

void EventLoop::QueueInLoop(const Functor& cb) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        pending_functors_.push_back(cb);
        ++pending_functor_count_;
    }

    if (calling_pending_functors_ || !IsInLoopThread()) {
        watcher_->Notify();
    }
}

void EventLoop::DoPendingFunctors() {
    std::vector<Functor> functors;
    calling_pending_functors_ = true;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        functors.swap(pending_functors_);
    }

    for (size_t i = 0; i < functors.size(); ++i) {
        functors[i]();
        --pending_functor_count_;
    }

    calling_pending_functors_ = false;
}

void EventLoop::AssertInLoopThread() const {
    assert(IsInLoopThread());
}
}
