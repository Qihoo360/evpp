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

    // ��һ�����е�event_base���󴴽�EventLoop��
    // �����Ϳ��Խ�evpp::EventLoop�����Ƕ�뵽��Ļ���libevent�����п���С�
    // ��Ҫ��ϸ���� event_base_��watcher_�ȶ�����ͷ�����
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
        context_ = c;
    }
    const Any& context() const {
        return context_;
    }
    bool running() const {
        return running_;
    }
    int pending_functor_count() const {
        return pending_functor_count_.load();
    }
private:
    void StopInLoop();
    void Init();
    void DoPendingFunctors();

private:
    struct event_base* evbase_;
    bool create_evbase_myself_;
    std::thread::id tid_;
    Any context_;
    bool running_;

    std::mutex mutex_;
    std::shared_ptr<PipeEventWatcher> watcher_;
    std::vector<Functor> pending_functors_; // @Guarded By mutex_
    bool calling_pending_functors_;

    std::atomic<int> pending_functor_count_;
};
}
