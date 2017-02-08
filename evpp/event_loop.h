#pragma once

#include <vector>
#include <thread>
#include <mutex>
#include <atomic>

#include "evpp/inner_pre.h"
#include "evpp/libevent_watcher.h"
#include "evpp/duration.h"
#include "evpp/any.h"
#include "evpp/invoke_timer.h"

namespace evpp {

class EVPP_EXPORT EventLoop {
public:
    typedef std::function<void()> Functor;
public:
    EventLoop();

    // 从一个已有的event_base对象创建EventLoop，
    // 这样就可以将evpp::EventLoop方便的嵌入到别的基于libevent的已有框架中。
    // 需要仔细处理 event_base_、watcher_等对象的释放问题
    explicit EventLoop(struct event_base* base);
    ~EventLoop();

    void Run();
    void Stop();
    void AfterFork(); // Reinitialized the event base after a fork

    InvokeTimerPtr RunAfter(double delay_ms, const Functor& f);
    InvokeTimerPtr RunAfter(Duration delay, const Functor& f);

    // RunEvery executes Functor f every period interval time.
    InvokeTimerPtr RunEvery(Duration interval, const Functor& f);

    void RunInLoop(const Functor& handler);
    void QueueInLoop(const Functor& handler);

    struct event_base* event_base() {
        return evbase_;
    }
    bool IsInLoopThread() const;
    void AssertInLoopThread() const;
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
    bool running() const {
        return running_;
    }
    bool IsRunning() const {
        return running();
    }
    bool IsStopped() const {
        return !running();
    }
    int pending_functor_count() const {
        return pending_functor_count_.load();
    }
    const std::thread::id& tid() const {
        return tid_;
    }
private:
    void Init();
    void InitEventWatcher();
    void StopInLoop();
    void DoPendingFunctors();

private:
    struct event_base* evbase_;
    bool create_evbase_myself_;
    std::thread::id tid_;
    enum { kContextCount = 16, };
    Any context_[kContextCount];
    bool running_;

    std::mutex mutex_;
    std::shared_ptr<PipeEventWatcher> watcher_;
    std::vector<Functor> pending_functors_; // @Guarded By mutex_
    bool calling_pending_functors_;

    std::atomic<int> pending_functor_count_;
};
}
