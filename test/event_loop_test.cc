
#include "evpp/exp.h"
#include "test_common.h"

#include "evpp/libevent_headers.h"
#include "evpp/libevent_watcher.h"
#include "evpp/event_loop.h"
#include "evpp/timestamp.h"

#include <thread>

namespace evloop
{
    static xstd::shared_ptr<evpp::EventLoop> loop;
    static evpp::Duration delay(1.0);
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
    usleep(delay.Microseconds());
    evpp::Timestamp start = evpp::Timestamp::Now();
    loop->RunAfter(delay, &Handle);
    th.join();
    evpp::Duration cost = evpp::Timestamp::Now() - start;
    H_TEST_ASSERT(delay <= cost);
    H_TEST_ASSERT(g_event_handler_called);
}

