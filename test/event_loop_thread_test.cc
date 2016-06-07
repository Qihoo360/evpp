#include "test_common.h"

#include "evpp/libevent_headers.h"
#include "evpp/libevent_watcher.h"
#include "evpp/event_loop.h"
#include "evpp/event_loop_thread.h"

#include <atomic>

namespace
{
    static bool g_timeout = false;
    static std::atomic<int> g_count;
    static void OnTimeout()
    {
        g_timeout = true;
    }

    static void OnCount()
    {
        g_count++;
    }
}


TEST_UNIT(testEventLoopThread)
{
    g_count.store(0);
    evpp::EventLoopThread t;
    t.Start();
    uint64_t begin_us = evpp::utcmicrosecond();
    t.event_loop()->RunAfter(1000 * 2, &OnTimeout);
    while (!g_timeout)
    {
        usleep(1);
    }
    uint64_t end_us = evpp::utcmicrosecond();
    H_TEST_ASSERT(end_us - begin_us >= 2000 * 1000);
    t.event_loop()->RunInLoop(&OnCount);
    t.event_loop()->RunInLoop(&OnCount);
    t.event_loop()->RunInLoop(&OnCount);
    t.event_loop()->RunInLoop(&OnCount);
    t.Stop(true);
    H_TEST_ASSERT(g_count == 4);
}
