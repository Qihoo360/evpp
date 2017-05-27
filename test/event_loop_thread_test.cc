#include "test_common.h"

#include <evpp/libevent.h>
#include <evpp/event_watcher.h>
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
    std::unique_ptr<evpp::EventLoopThread> t(new evpp::EventLoopThread);
    t->Start(true);
    usleep(1000);
    evpp::Timestamp begin = evpp::Timestamp::Now();
    t->loop()->RunAfter(delay, &OnTimeout);

    while (!g_timeout) {
        usleep(1);
    }

    evpp::Duration cost = evpp::Timestamp::Now() - begin;
    H_TEST_ASSERT(delay <= cost);
    t->loop()->RunInLoop(&OnCount);
    t->loop()->RunInLoop(&OnCount);
    t->loop()->RunInLoop(&OnCount);
    t->loop()->RunInLoop(&OnCount);
    t->Stop(true);
    t.reset();
    H_TEST_ASSERT(g_count == 4);
    H_TEST_ASSERT(evpp::GetActiveEventCount() == 0);
}
