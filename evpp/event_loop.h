#pragma once

#include <vector>
#include <thread>
#include <mutex>

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
        explicit EventLoop(struct event_base *base);
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
        void AddAfterLoopFunctor(const Functor& handler);

        struct event_base* event_base() { return event_base_; }
        bool IsInLoopThread() const;
        void AssertInLoopThread() const;
        void set_context(const Any& c) { context_ = c; }
        const Any& context() const { return context_; }

    private:
        void StopInLoop();
        void Init();
        void DoPendingFunctors();
        void DoAfterLoopFunctors();

    private:
        struct event_base* event_base_;
        std::thread::id tid_;
        Any context_;

        std::mutex mutex_;
        std::shared_ptr<PipeEventWatcher> watcher_;
        std::vector<Functor> pending_functors_; // @Guarded By mutex_
        bool calling_pending_functors_;
        std::vector<Functor> after_loop_functors_; // @Guarded By mutex_
    };
}
