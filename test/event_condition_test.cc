
#include "evpp/exp.h"
#include "test_common.h"

#include "evpp/libevent_headers.h"
#include "evpp/libevent_watcher.h"
#include <thread>

namespace
{
    static bool g_event_handler_called = false;
    static void Handle(struct event_base* base)
    {
        g_event_handler_called = true;
        event_base_loopexit(base, 0);
    }

    static void MyEventThread(struct event_base* base, evpp::PipeEventWatcher* ev)
    {
        if (ev->Init())
        {
            ev->Watch((uint64_t)0);
        }
        event_base_loop(base, 0);
    }
}

TEST_UNIT(testPipeEventWatcher)
{
    struct event_base* base = event_base_new();
    evpp::PipeEventWatcher ev(base, xstd::bind(Handle, base));
    std::thread th(MyEventThread, base, &ev);
    ::usleep(1000 * 100);
    ev.Notify();
    th.join();
    event_base_free(base);
    H_TEST_ASSERT(g_event_handler_called == true);
}

