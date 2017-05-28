#include "test_common.h"

#include <signal.h>

#include <thread>

#include <evpp/libevent.h>
#include <evpp/event_watcher.h>
#include <evpp/timestamp.h>
#include <evpp/event_loop.h>
#include <evpp/event_loop_thread.h>

// 
// namespace evtimer {
// static evpp::Duration g_timeout(1.0); // 1s
// static bool g_event_handler_called = false;
// static void Handle(struct event_base* base) {
//     g_event_handler_called = true;
//     event_base_loopexit(base, 0);
// }
// 
// static void MyEventThread(struct event_base* base, evpp::TimerEventWatcher* ev) {
//     ev->Init();
//     ev->AsyncWait();
//     event_base_loop(base, 0);
//     delete ev; // make sure to initialize and delete in the same thread.
// }
// }
// 
// TEST_UNIT(testTimerEventWatcher) {
//     using namespace evtimer;
//     struct event_base* base = event_base_new();
//     evpp::Timestamp start = evpp::Timestamp::Now();
//     evpp::TimerEventWatcher* ev(new evpp::TimerEventWatcher(base, std::bind(&Handle, base), g_timeout));
//     std::thread th(MyEventThread, base, ev);
//     th.join();
//     evpp::Duration cost = evpp::Timestamp::Now() - start;
//     H_TEST_ASSERT(g_timeout <= cost);
//     H_TEST_ASSERT(g_event_handler_called);
//     event_base_free(base);
//     H_TEST_ASSERT(evpp::GetActiveEventCount() == 0);
// }


namespace evtimer {
static evpp::Duration g_timeout(1.0); // 1s
static bool g_event_handler_called = false;
static void Handle(evpp::EventLoop* loop) {
    g_event_handler_called = true;
    loop->Stop();
}

static void MyEventThread(evpp::EventLoop* loop, evpp::TimerEventWatcher* ev) {
    ev->Init();
    ev->AsyncWait();
    loop->Run();
    delete ev; // make sure to initialize and delete in the same thread.
}
}

TEST_UNIT(testTimerEventWatcher) {
    using namespace evtimer;
    std::unique_ptr<evpp::EventLoop> loop(new evpp::EventLoop);
    evpp::Timestamp start = evpp::Timestamp::Now();
    evpp::TimerEventWatcher* ev(new evpp::TimerEventWatcher(loop.get(), std::bind(&Handle, loop.get()), g_timeout));
    std::thread th(MyEventThread, loop.get(), ev);
    th.join();
    evpp::Duration cost = evpp::Timestamp::Now() - start;
    H_TEST_ASSERT(g_timeout <= cost);
    H_TEST_ASSERT(g_event_handler_called);
    loop.reset();
    H_TEST_ASSERT(evpp::GetActiveEventCount() == 0);
}

TEST_UNIT(testsocketpair) {
    evpp_socket_t sockpair[2];
    memset(sockpair, 0, sizeof(sockpair[0] * 2));
    int r = evutil_socketpair(AF_UNIX, SOCK_STREAM, 0, sockpair);
    H_TEST_ASSERT(r >= 0);
    H_TEST_ASSERT(sockpair[0] > 0);
    H_TEST_ASSERT(sockpair[1] > 0);
    EVUTIL_CLOSESOCKET(sockpair[0]);
    EVUTIL_CLOSESOCKET(sockpair[1]);
}

namespace evsignal {
static evpp::SignalEventWatcher* ev = nullptr;
static bool g_event_handler_called = false;
static void Handle(evpp::EventLoopThread* thread) {
    LOG_INFO << "SIGINT caught.";
    g_event_handler_called = true;
    thread->Stop();
    delete ev; // make sure to initialize and delete in the same thread.
    ev = nullptr;
}

static void WatchSignalInt() {
    ev->Init();
    ev->AsyncWait();
}
}

TEST_UNIT(testSignalEventWatcher) {
    using namespace evsignal;
    std::shared_ptr<evpp::EventLoopThread> thread(new evpp::EventLoopThread);
    thread->Start(true);
    evpp::EventLoop* loop = thread->loop();
    ev = new evpp::SignalEventWatcher(SIGINT, loop, std::bind(&Handle, thread.get()));
    loop->RunInLoop(&WatchSignalInt);
    auto f = []() {
        LOG_INFO << "Send SIGINT ...";
#ifdef H_OS_WINDOWS
        raise(SIGINT);
#else
        kill(getpid(), SIGINT);
#endif
    };
    loop->RunAfter(evpp::Duration(0.1), f);
    while (!thread->IsStopped()) {
        usleep(1);
    }
    thread.reset();
    H_TEST_ASSERT(g_event_handler_called);
    H_TEST_ASSERT(evpp::GetActiveEventCount() == 0);
}
