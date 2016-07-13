#include "test_common.h"

#include <thread>

#include <evpp/exp.h>
#include <evpp/libevent_headers.h>
#include <evpp/libevent_watcher.h>
#include <evpp/timestamp.h>



namespace evtimer {
static evpp::Duration g_timeout(1.0); // 1s
static bool g_event_handler_called = false;
static void Handle(struct event_base* base) {
    g_event_handler_called = true;
    event_base_loopexit(base, 0);
}

static void MyEventThread(struct event_base* base, evpp::TimerEventWatcher* ev) {
    ev->Init();
    ev->AsyncWait();
    event_base_loop(base, 0);
}
}

TEST_UNIT(testTimerEventWatcher) {
    using namespace evtimer;
    struct event_base* base = event_base_new();
    evpp::Timestamp start = evpp::Timestamp::Now();
    std::shared_ptr<evpp::TimerEventWatcher> ev(new evpp::TimerEventWatcher(base, std::bind(&Handle, base), g_timeout));
    std::thread th(MyEventThread, base, ev.get());
    th.join();
    evpp::Duration cost = evpp::Timestamp::Now() - start;
    H_TEST_ASSERT(g_timeout <= cost);
    H_TEST_ASSERT(g_event_handler_called);
    ev.reset();
    event_base_free(base);
}

TEST_UNIT(testsocketpair) {
    int sockpair[2];
    memset(sockpair, 0, sizeof(sockpair[0] * 2));

    int r = evutil_socketpair(AF_UNIX, SOCK_STREAM, 0, sockpair);
    H_TEST_ASSERT(r >= 0);
    EVUTIL_CLOSESOCKET(sockpair[0]);
    EVUTIL_CLOSESOCKET(sockpair[1]);
}

