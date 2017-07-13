#pragma once

#include <vector>
#include <thread>
#include <mutex>
#include <atomic>

#include "evpp/inner_pre.h"
#include "evpp/event_watcher.h"
#include "evpp/duration.h"
#include "evpp/any.h"
#include "evpp/invoke_timer.h"
#include "evpp/server_status.h"

#ifdef H_HAVE_BOOST
#include <boost/lockfree/queue.hpp>
#elif defined(H_HAVE_CAMERON314_CONCURRENTQUEUE)

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#include <concurrentqueue/concurrentqueue.h>
#pragma GCC diagnostic pop
#else
#include <concurrentqueue/concurrentqueue.h>
#endif // __GNUC__

#endif

namespace evpp {

// This is the IO Event driving kernel. Reactor model.
// This class is a wrapper of event_base but not only a wrapper.
// It provides a simple way to run a IO Event driving loop.
// One thread one loop.
class EVPP_EXPORT EventLoop : public ServerStatus {
public:
    typedef std::function<void()> Functor;
public:
    EventLoop();

    // Build an EventLoop using an existing event_base object,
    // so we can embed an EventLoop object into the old applications based on libevent
    // NOTE: Be careful to deal with the destructing work of event_base_ and watcher_ objects.
    explicit EventLoop(struct event_base* base);
    ~EventLoop();

    // @brief Run the IO Event driving loop forever
    // @note It must be called in the IO Event thread
    void Run();

    // @brief Stop the event loop
    void Stop();

    // @brief Reinitialize some data fields after a fork
    void AfterFork();

    InvokeTimerPtr RunAfter(double delay_ms, const Functor& f);
    InvokeTimerPtr RunAfter(Duration delay, const Functor& f);

    // RunEvery executes Functor f every period interval time.
    InvokeTimerPtr RunEvery(Duration interval, const Functor& f);

    void RunInLoop(const Functor& handler);
    void QueueInLoop(const Functor& handler);

public:

    InvokeTimerPtr RunAfter(double delay_ms, Functor&& f);
    InvokeTimerPtr RunAfter(Duration delay, Functor&& f);

    // RunEvery executes Functor f every period interval time.
    InvokeTimerPtr RunEvery(Duration interval, Functor&& f);

    void RunInLoop(Functor&& handler);
    void QueueInLoop(Functor&& handler);

    // Getter and Setter
public:
    struct event_base* event_base() {
        return evbase_;
    }
    bool IsInLoopThread() const {
        return tid_ == std::this_thread::get_id();
    }
    void set_context(const Any& c) {
        context_[0] = c;
    }
    const Any& context() const {
        return context_[0];
    }
    void set_context(int index, const Any& c) {
        assert(index < kContextCount && index >= 0);
        context_[index] = c;
    }
    const Any& context(int index) const {
        assert(index < kContextCount && index >= 0);
        return context_[index];
    }
    int pending_functor_count() const {
        return pending_functor_count_.load();
    }
    const std::thread::id& tid() const {
        return tid_;
    }
private:
    void Init();
    void InitNotifyPipeWatcher();
    void StopInLoop();
    void DoPendingFunctors();
    size_t GetPendingQueueSize();
    bool IsPendingQueueEmpty();
private:
    struct event_base* evbase_;
    bool create_evbase_myself_;
    std::thread::id tid_;
    enum { kContextCount = 16, };
    Any context_[kContextCount];

    std::mutex mutex_;
    // We use this to notify the thread when we put a task into the pending_functors_ queue
    std::shared_ptr<PipeEventWatcher> watcher_;
    // When we put a task into the pending_functors_ queue,
    // we need to notify the thread to execute it. But we don't want to notify repeatedly.
    std::atomic<bool> notified_;
#ifdef H_HAVE_BOOST
    boost::lockfree::queue<Functor*>* pending_functors_;
#elif defined(H_HAVE_CAMERON314_CONCURRENTQUEUE)
    moodycamel::ConcurrentQueue<Functor>* pending_functors_;
#else
    std::vector<Functor>* pending_functors_; // @Guarded By mutex_
#endif

    std::atomic<int> pending_functor_count_;
};
}
