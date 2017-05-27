
#include "test_common.h"

#include <evpp/libevent.h>
#include <evpp/event_watcher.h>
#include <evpp/event_loop.h>
#include <evpp/timestamp.h>

#include <thread>

namespace evloop {
static std::shared_ptr<evpp::EventLoop> loop;
static evpp::Duration delay(1.0);
static evpp::Duration cancel_delay(0.5);
static bool event_handler_called = false;
static void Close() {
    loop->Stop();
}

static void Handle() {
    event_handler_called = true;
}

static void MyEventThread() {
    LOG_INFO << "EventLoop is running ...";
    loop = std::shared_ptr<evpp::EventLoop>(new evpp::EventLoop);
    loop->Run();
}
}

TEST_UNIT(testInvokerTimerCancel) {
    using namespace evloop;
    std::thread th(MyEventThread);
    usleep(delay.Microseconds());
    evpp::Timestamp start = evpp::Timestamp::Now();
    loop->RunAfter(delay, &Close);
    evpp::InvokeTimerPtr timer = loop->RunAfter(delay, &Handle);
    usleep(cancel_delay.Microseconds());
    timer->Cancel();
    th.join();
    timer.reset();
    loop.reset();
    evpp::Duration cost = evpp::Timestamp::Now() - start;
    H_TEST_ASSERT(delay <= cost);
    H_TEST_ASSERT(!event_handler_called);
    H_TEST_ASSERT(evpp::GetActiveEventCount() == 0);
}

