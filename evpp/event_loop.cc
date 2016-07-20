#include "evpp/inner_pre.h"

#include "evpp/libevent_headers.h"
#include "evpp/libevent_watcher.h"
#include "evpp/event_loop.h"
#include "evpp/invoke_timer.h"

namespace evpp {
EventLoop::EventLoop() {
#if LIBEVENT_VERSION_NUMBER >= 0x02001500
    struct event_config* cfg = event_config_new();

    if (cfg) {
        // Does not cache time to get a preciser timer
        event_config_set_flag(cfg, EVENT_BASE_FLAG_NO_CACHE_TIME);
        event_base_ = event_base_new_with_config(cfg);
        event_config_free(cfg);
    }

#else
    event_base_ = event_base_new();
#endif
    Init();
}

EventLoop::EventLoop(struct event_base* base)
    : event_base_(base) {
    Init();
}

EventLoop::~EventLoop() {
    watcher_.reset(); // 在 event_base 之前释放

    if (event_base_ != NULL) {
        event_base_free(event_base_);
        event_base_ = NULL;
    }
}

void EventLoop::Init() {
    running_ = false;
    tid_ = std::this_thread::get_id(); // The default thread id
    calling_pending_functors_ = false;
    watcher_.reset(new PipeEventWatcher(this, std::bind(&EventLoop::DoPendingFunctors, this)));
    watcher_->Init();
    watcher_->AsyncWait();
}

void EventLoop::Run() {
    running_ = true;
    tid_ = std::this_thread::get_id(); // The actual thread id
    int rc = event_base_dispatch(event_base_);

    if (rc == 1) {
        LOG_ERROR << "event_base_dispatch error: no event registered";
        return;
    } else if (rc == -1) {
        LOG_FATAL << "event_base_dispatch error";
        return;
    }

    //LOG_TRACE << "EventLoop stopped, tid: " << std::this_thread::get_id();
    running_ = false;
}

bool EventLoop::IsInLoopThread() const {
    std::thread::id cur_tid = std::this_thread::get_id();
    return cur_tid == tid_;
}

void EventLoop::Stop() {
    RunInLoop(std::bind(&EventLoop::StopInLoop, this));
}

void EventLoop::StopInLoop() {
    LOG_TRACE << "EventLoop is stopping now, tid=" << std::this_thread::get_id();

    for (;;) {
        DoPendingFunctors();

        std::lock_guard<std::mutex> lock(mutex_);

        if (pending_functors_.empty()) {
            break;
        }
    }

    //TODO make sure all the event in event_base stopped.
    timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 500 * 1000; //500ms
    event_base_loopexit(event_base_, &tv);
}

void EventLoop::AfterFork() {
    int rc = event_reinit(event_base_);
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
    //LOG_INFO << "EventLoop::RunInLoop tid=" << std::this_thread::get_id() << " IsInLoopThread=" << IsInLoopThread();
    if (IsInLoopThread()) {
        functor();
    } else {
        QueueInLoop(functor);
    }
}

void EventLoop::QueueInLoop(const Functor& cb) {
    LOG_INFO << "EventLoop::QueueInLoop tid=" << std::this_thread::get_id() << " IsInLoopThread=" << IsInLoopThread();
    {
        std::lock_guard<std::mutex> lock(mutex_);
        pending_functors_.push_back(cb);
    }

    if (calling_pending_functors_ || !IsInLoopThread()) {
        //LOG_INFO << "EventLoop::QueueInLoop tid=" << std::this_thread::get_id() << " watcher_->Notify()";
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
    }

    calling_pending_functors_ = false;
}

void EventLoop::AssertInLoopThread() const {
    assert(IsInLoopThread());
}
}
