#include "evpp/inner_pre.h"

#include "evpp/libevent.h"
#include "evpp/event_watcher.h"
#include "evpp/event_loop.h"
#include "evpp/invoke_timer.h"
#include "folly/synchronization/Baton.h"
#include "folly/io/async/EventUtil.h"

namespace evpp {
EventLoop::EventLoop()
    : create_evbase_myself_(true), notified_(false), pending_functor_count_(0) {
    DLOG_TRACE;
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
    : evbase_(base), create_evbase_myself_(false), notified_(false), pending_functor_count_(0) {
    DLOG_TRACE;
    Init();

    // When we build an EventLoop instance from an existing event_base
    // object, we will never call EventLoop::Run() method.
    // So we need to watch the task queue here.
    bool rc = watcher_->AsyncWait();
    assert(rc);
    if (!rc) {
        LOG_FATAL << "PipeEventWatcher init failed.";
    }
    status_.store(kRunning);
}

EventLoop::~EventLoop() {
    DLOG_TRACE;
    watcher_.reset();

    if (evbase_ != nullptr && create_evbase_myself_) {
        event_base_free(evbase_);
        evbase_ = nullptr;
    }

    //delete pending_functors_;
    //pending_functors_ = nullptr;
}

void EventLoop::Init() {
    DLOG_TRACE;
    status_.store(kInitializing);
#ifdef H_HAVE_BOOST
    const size_t kPendingFunctorCount = 1024 * 16;
    this->pending_functors_ = new boost::lockfree::queue<Functor*>(kPendingFunctorCount);
#elif defined(H_HAVE_CAMERON314_CONCURRENTQUEUE)
    this->pending_functors_ = new moodycamel::ConcurrentQueue<Functor>();
#else
    //this->pending_functors_ = new std::vector<Functor>();
#endif

    tid_ = std::this_thread::get_id(); // The default thread id

    InitNotifyPipeWatcher();

    status_.store(kInitialized);
}

void EventLoop::InitNotifyPipeWatcher() {
    // Initialized task queue notify pipe watcher
    //watcher_.reset(new PipeEventWatcher(this, std::bind(&EventLoop::DoPendingFunctors, this)));
    watcher_.reset(new PipeEventWatcher(this, []() {}));
    int rc = watcher_->Init();
    assert(rc);
    if (!rc) {
        LOG_FATAL << "PipeEventWatcher init failed.";
    }
}

void EventLoop::WaitUntilRunning() {
    folly::Baton<> bat;
    RunInLoop([baton = std::ref(bat)]() {
        baton.get().post();
    });
    bat.wait();
}

void EventLoop::RunOnce() {
    int res = 0;
    if (status_ == kStopped) {
        return;
    }
    DoBeforeLoop();
    res = event_base_loop(evbase_, EVLOOP_ONCE | EVLOOP_NONBLOCK);
    if (res == -1) {
        int serrno = errno;
        LOG_ERROR << "event_base_dispatch error " << serrno << " thread_id=" << pthread_self();
    }
    DoPendingFunctors();
}

void EventLoop::Run() {
    DLOG_TRACE;
    status_.store(kStarting);
    tid_ = std::this_thread::get_id(); // The actual thread id

    int rc = watcher_->AsyncWait();
    assert(rc);
    if (!rc) {
        LOG_FATAL << "PipeEventWatcher init failed.";
    }

    // After everything have initialized, we set the status to kRunning
    status_.store(kRunning);

    while (status_ != kStopped) {
        //rc = event_base_dispatch(evbase_);
        DoBeforeLoop();
        if (GetPendingQueueSize() == 0) {
            LOG_TRACE << "pending size = 0, block";
            rc = event_base_loop(evbase_, EVLOOP_ONCE);//block
        } else {
            rc = event_base_loop(evbase_, EVLOOP_ONCE | EVLOOP_NONBLOCK);
        }
        if (rc == 1) {
            LOG_ERROR << "event_base_dispatch error: no event registered";
        } else if (rc == -1) {
            int serrno = errno;
            LOG_ERROR << "event_base_dispatch error " << serrno << " " << strerror(serrno) << " threadid=" << pthread_self();
        }
        DoPendingFunctors();
    }
    DoPendingFunctors();

    // Make sure watcher_ does construct, initialize and destruct in the same thread.
    watcher_.reset();
    DLOG_TRACE << "EventLoop stopped, tid=" << std::this_thread::get_id();

}

void EventLoop::Stop() {
    DLOG_TRACE;
    assert(status_.load() == kRunning);
    status_.store(kStopping);
    DLOG_TRACE << "EventLoop::Stop";
    QueueInLoop(std::bind(&EventLoop::StopInLoop, this));
}

void EventLoop::StopInLoop() {
    DLOG_TRACE << "EventLoop is stopping now, tid=" << std::this_thread::get_id();
    assert(status_.load() == kStopping);

    auto f = [this]() {
        for (int i = 0;; i++) {
            DLOG_TRACE << "calling DoPendingFunctors index=" << i;
            DoPendingFunctors();
            if (IsPendingQueueEmpty()) {
                break;
            }
        }
    };

    DLOG_TRACE << "before event_base_loopexit, we invoke DoPendingFunctors";

    f();

    DLOG_TRACE << "start event_base_loopexit";
    event_base_loopexit(evbase_, nullptr);
    DLOG_TRACE << "after event_base_loopexit, we invoke DoPendingFunctors";

    f();
    status_.store(kStopped);

    DLOG_TRACE << "end of StopInLoop";
}

void EventLoop::AfterFork() {
    int rc = event_reinit(evbase_);
    assert(rc == 0);

    if (rc != 0) {
        LOG_FATAL << "event_reinit failed!";
        abort();
    }

    // We create EventLoopThread and initialize it in father process,
    // but we use it in child process.
    // If we have only one child process, everything goes well.
    //
    // But if we have multi child processes, something goes wrong.
    // Because EventLoop::watcher_ is created and initialized in father process
    // all children processes inherited father's pipe.
    //
    // When we use the pipe to do a notification in one child process
    // the notification may be received by another child process randomly.
    //
    // So we need to reinitialize the watcher_
    InitNotifyPipeWatcher();
}

InvokeTimerPtr EventLoop::RunAfter(double delay_ms, const Functor& f) {
    DLOG_TRACE;
    return RunAfter(Duration(delay_ms / 1000.0), f);
}

InvokeTimerPtr EventLoop::RunAfter(double delay_ms, Functor&& f) {
    DLOG_TRACE;
    return RunAfter(Duration(delay_ms / 1000.0), std::move(f));
}

InvokeTimerPtr EventLoop::RunAfter(Duration delay, const Functor& f) {
    DLOG_TRACE;
    std::shared_ptr<InvokeTimer> t = InvokeTimer::Create(this, delay, f, false);
    t->Start();
    return t;
}

InvokeTimerPtr EventLoop::RunAfter(Duration delay, Functor&& f) {
    DLOG_TRACE;
    std::shared_ptr<InvokeTimer> t = InvokeTimer::Create(this, delay, std::move(f), false);
    t->Start();
    return t;
}

evpp::InvokeTimerPtr EventLoop::RunEvery(Duration interval, const Functor& f) {
    DLOG_TRACE;
    std::shared_ptr<InvokeTimer> t = InvokeTimer::Create(this, interval, f, true);
    t->Start();
    return t;
}

evpp::InvokeTimerPtr EventLoop::RunEvery(Duration interval, Functor&& f) {
    DLOG_TRACE;
    std::shared_ptr<InvokeTimer> t = InvokeTimer::Create(this, interval, std::move(f), true);
    t->Start();
    return t;
}

void EventLoop::drive() {
    RunOnce();
}

void EventLoop::attachTimeoutManager(folly::AsyncTimeout* obj, folly::AsyncTimeout::InternalEnum internal) {
    struct event* ev = obj->getEvent();
    assert(ev->ev_base == nullptr);

    event_base_set(event_base(), ev);
    if (internal == folly::AsyncTimeout::InternalEnum::INTERNAL) {
        // Set the EVLIST_INTERNAL flag
        folly::event_ref_flags(ev) |= EVLIST_INTERNAL;
    }
}
bool EventLoop::isInTimeoutManagerThread() {
    return IsInLoopThread();
}
void EventLoop::cancelTimeout(folly::AsyncTimeout* obj) {
    assert(isInTimeoutManagerThread());
    struct event* ev = obj->getEvent();
    if (folly::EventUtil::isEventRegistered(ev)) {
        event_del(ev);
    }
}
void EventLoop::detachTimeoutManager(folly::AsyncTimeout* obj) {
    cancelTimeout(obj);
    struct event* ev = obj->getEvent();
    ev->ev_base = nullptr;
}

bool EventLoop::scheduleTimeout(folly::AsyncTimeout* obj,
                                timeout_type timeout) {
    // Set up the timeval and add the event
    struct timeval tv;
    tv.tv_sec = long(timeout.count() / 1000LL);
    tv.tv_usec = long((timeout.count() % 1000LL) * 1000LL);

    struct event* ev = obj->getEvent();
    if (event_add(ev, &tv) < 0) {
        LOG(ERROR) << "EventBase: failed to schedule timeout: " << strerror(errno);
        return false;
    }

    return true;
}

void EventLoop::RunInLoop(const Functor& functor) {
    DLOG_TRACE;
    if (IsRunning() && IsInLoopThread()) {
        functor();
    } else {
        QueueInLoop(functor);
    }
}

void EventLoop::RunBeforeLoop(Functor&& functor) {
    assert(IsInLoopThread());
    run_before_loop_functors_.emplace_back(std::move(functor));
}

void EventLoop::DoBeforeLoop() {
    assert(IsInLoopThread());
    while (!run_before_loop_functors_.empty()) {
        Functor f(std::move(run_before_loop_functors_.front()));
        run_before_loop_functors_.pop_front();
        f();
    }
}

void EventLoop::RunInLoop(Functor&& functor) {
    DLOG_TRACE;
    if (IsRunning() && IsInLoopThread()) {
        functor();
    } else {
        QueueInLoop(std::move(functor));
    }
}

void EventLoop::QueueInLoop(const Functor& cb) {
    DLOG_TRACE << "pending_functor_count_=" << pending_functor_count_ << " PendingQueueSize=" << GetPendingQueueSize() << " notified_=" << notified_.load();
    {
#ifdef H_HAVE_BOOST
        auto f = new Functor(cb);
        while (!pending_functors_->push(f)) {
        }
#elif defined(H_HAVE_CAMERON314_CONCURRENTQUEUE)
        while (!pending_functors_->enqueue(cb)) {
        }
#else
        {
            folly::SpinLockGuard g(spinlock_);
            pending_functors_.emplace_back(cb);
        }
#endif
    }
    ++pending_functor_count_;
    DLOG_TRACE << "queued a new Functor. pending_functor_count_=" << pending_functor_count_ << " PendingQueueSize=" << GetPendingQueueSize() << " notified_=" << notified_.load();
    if (!notified_.load()) {
        DLOG_TRACE << "call watcher_->Nofity() notified_.store(true)";

        // We must set notified_ to true before calling `watcher_->Nodify()`
        // otherwise there is a change that:
        //  1. We called watcher_- > Nodify() on thread1
        //  2. On thread2 we watched this event, so wakeup the CPU changed to run this EventLoop on thread2 and executed all the pending task
        //  3. Then the CPU changed to run on thread1 and set notified_ to true
        //  4. Then, some thread except thread2 call this QueueInLoop to push a task into the queue, and find notified_ is true, so there is no change to wakeup thread2 to execute this task
        notified_.store(true);

        // Sometimes one thread invoke EventLoop::QueueInLoop(...), but anther
        // thread is invoking EventLoop::Stop() to stop this loop. At this moment
        // this loop maybe is stopping and the watcher_ object maybe has been
        // released already.
        if (watcher_) {
            watcher_->Notify();
        } else {
            DLOG_TRACE << "status=" << StatusToString();
            assert(!IsRunning());
        }
    } else {
        DLOG_TRACE << "No need to call watcher_->Nofity()";
    }
}

void EventLoop::QueueInLoop(Functor&& cb) {
    DLOG_TRACE << "pending_functor_count_=" << pending_functor_count_ << " PendingQueueSize=" << GetPendingQueueSize() << " notified_=" << notified_.load();
    {
#ifdef H_HAVE_BOOST
        auto f = new Functor(std::move(cb)); // TODO Add test code for it
        while (!pending_functors_->push(f)) {
        }
#elif defined(H_HAVE_CAMERON314_CONCURRENTQUEUE)
        while (!pending_functors_->enqueue(std::move(cb))) {
        }
#else
        folly::SpinLockGuard g(spinlock_);
        pending_functors_.emplace_back(std::move(cb));
#endif
    }
    ++pending_functor_count_;
    DLOG_TRACE << "queued a new Functor. pending_functor_count_=" << pending_functor_count_ << " PendingQueueSize=" << GetPendingQueueSize() << " notified_=" << notified_.load();
    if (!notified_.load()) {
        DLOG_TRACE << "call watcher_->Nofity() notified_.store(true)";
        notified_.store(true);
        if (watcher_) {
            watcher_->Notify();
        } else {
            DLOG_TRACE << "watcher_ is empty, maybe we call EventLoop::QueueInLoop on a stopped EventLoop. status=" << StatusToString();
            assert(!IsRunning());
        }
    } else {
        DLOG_TRACE << "No need to call watcher_->Nofity()";
    }
}

void EventLoop::DoPendingFunctors() {
    DLOG_TRACE << "pending_functor_count_=" << pending_functor_count_ << " PendingQueueSize=" << GetPendingQueueSize() << " notified_=" << notified_.load();

#ifdef H_HAVE_BOOST
    notified_.store(false);
    Functor* f = nullptr;
    while (pending_functors_->pop(f)) {
        (*f)();
        delete f;
        --pending_functor_count_;
    }
#elif defined(H_HAVE_CAMERON314_CONCURRENTQUEUE)
    notified_.store(false);
    Functor f;
    while (pending_functors_->try_dequeue(f)) {
        f();
        --pending_functor_count_;
    }
#else
    notified_.store(false);
    while (true) {
        Functor f;
        {
            folly::SpinLockGuard g(spinlock_);
            if (pending_functors_.size() == 0) {
                break;
            }
            f = std::move(pending_functors_.front());
            pending_functors_.pop_front();
        }
        f();
        --pending_functor_count_;
    }
    DLOG_TRACE << "pending_functor_count_=" << pending_functor_count_ << " PendingQueueSize=" << GetPendingQueueSize() << " notified_=" << notified_.load();
#endif
}

size_t EventLoop::GetPendingQueueSize() {
#ifdef H_HAVE_BOOST
    return static_cast<size_t>(pending_functor_count_.load());
#elif defined(H_HAVE_CAMERON314_CONCURRENTQUEUE)
    return pending_functors_->size_approx();
#else
    return pending_functors_.size();
#endif
}

bool EventLoop::IsPendingQueueEmpty() {
#ifdef H_HAVE_BOOST
    return pending_functors_->empty();
#elif defined(H_HAVE_CAMERON314_CONCURRENTQUEUE)
    return pending_functors_->size_approx() == 0;
#else
    return pending_functors_.empty();
#endif
}

}
