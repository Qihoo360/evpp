#include "test_common.h"

#include <evpp/libevent_headers.h>
#include <evpp/libevent_watcher.h>
#include <evpp/event_loop.h>
#include <evpp/event_loop_thread.h>
#include <evpp/timestamp.h>
#include <atomic>

namespace {
    static bool g_timeout = false;
    static std::atomic<int> g_count;
    static void OnTimeout() {
        g_timeout = true;
    }

    static void OnCount() {
        g_count++;
    }
}


TEST_UNIT(testEventLoopThread) {
    evpp::Duration delay(double(1.0)); // 1s
    g_count.store(0);
    evpp::EventLoopThread t;
    t.Start();
    evpp::Timestamp begin = evpp::Timestamp::Now();
    t.event_loop()->RunAfter(delay, &OnTimeout);
    while (!g_timeout) {
        usleep(1);
    }
    evpp::Duration cost = evpp::Timestamp::Now() - begin;
    H_TEST_ASSERT(delay <= cost);
    t.event_loop()->RunInLoop(&OnCount);
    t.event_loop()->RunInLoop(&OnCount);
    t.event_loop()->RunInLoop(&OnCount);
    t.event_loop()->RunInLoop(&OnCount);
    t.Stop(true);
    H_TEST_ASSERT(g_count == 4);
}
