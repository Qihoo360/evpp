
#include "evpp/exp.h"
#include "test_common.h"

#include "evpp/libevent_headers.h"
#include "evpp/libevent_watcher.h"
#include "evpp/event_loop.h"

#include <thread>

namespace evloop
{
    static xstd::shared_ptr<evpp::EventLoop> loop;
    static double delay_ms = 1000.0;
    static bool g_event_handler_called = false;
    static void Handle() {
        g_event_handler_called = true;
        loop->Stop();
    }

    static void MyEventThread() {
        LOG_INFO << "EventLoop is running ...";
        loop = xstd::shared_ptr<evpp::EventLoop>(new evpp::EventLoop);
        loop->Run();
    }
}

TEST_UNIT(testEventLoop)
{
    using namespace evloop;
    std::thread th(MyEventThread);
    usleep((uint32_t)(delay_ms * 1000));
    uint64_t start = evpp::utcmicrosecond();
    loop->RunAfter(delay_ms, &Handle);
    //loop->RunInLoop(&Handle);
    th.join();
    uint64_t end = evpp::utcmicrosecond();
    H_TEST_ASSERT(end - start >= delay_ms*1000);
    H_TEST_ASSERT(g_event_handler_called);
}

