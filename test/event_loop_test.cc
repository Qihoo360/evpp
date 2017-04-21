
#include "test_common.h"

#include <evpp/libevent_headers.h>
#include <evpp/event_watcher.h>
#include <evpp/event_loop.h>
#include <evpp/timestamp.h>

#include <thread>

namespace evloop {
static std::shared_ptr<evpp::EventLoop> loop;
static evpp::Duration delay(1.0);
static bool event_handler_called = false;
static void Handle(evpp::InvokeTimerPtr t) {
    event_handler_called = true;
    t->Cancel();
    loop->Stop();
}

static void MyEventThread() {
    LOG_INFO << "EventLoop is running ...";
    loop = std::shared_ptr<evpp::EventLoop>(new evpp::EventLoop);
    loop->Run();

    // Make sure the loop object is delete in its own thread.
    loop.reset();
}

static int periodic_run_count = 0;
static void PeriodicFunc() {
    periodic_run_count++;
    LOG_INFO << "PeriodicFunc is called , periodic_run_count=" << periodic_run_count;
}
}

TEST_UNIT(testEventLoop) {
    using namespace evloop;
    std::thread th(MyEventThread);
    usleep(delay.Microseconds());
    evpp::Timestamp start = evpp::Timestamp::Now();
    evpp::InvokeTimerPtr t = loop->RunEvery(evpp::Duration(0.3), &PeriodicFunc);
    loop->RunAfter(delay, std::bind(&Handle, t));
    th.join();
    t.reset();
    evpp::Duration cost = evpp::Timestamp::Now() - start;
    H_TEST_ASSERT(delay <= cost);
    H_TEST_ASSERT(event_handler_called);
    H_TEST_ASSERT(periodic_run_count == 3);
    H_TEST_ASSERT(evpp::GetActiveEventCount() == 0);
}


namespace {
    void OnTimer(evpp::EventLoop* loop) {

    }
}


TEST_UNIT(testEventLoop2) {
    evpp::EventLoop loop;
    auto timer = [&loop]() {
        auto close = [&loop]() {
            loop.Stop();
        };
        loop.QueueInLoop(close);
    };
    loop.RunAfter(evpp::Duration(0.5), timer);
    loop.Run();
    H_TEST_ASSERT(evpp::GetActiveEventCount() == 0);
}

// Test std::move of C++11
TEST_UNIT(testEventLoop3) {
    evpp::EventLoop loop;
    auto timer = [&loop]() {
        auto close = [&loop]() {
            loop.Stop();
        };
        loop.QueueInLoop(close);
    };
    loop.RunAfter(evpp::Duration(0.5), std::move(timer));
    loop.Run();
    H_TEST_ASSERT(evpp::GetActiveEventCount() == 0);
}




namespace {
evpp::EventLoop* loop = nullptr;
evpp::InvokeTimerPtr invoke_timer;
int count = 0;

void Run() {
    LOG_INFO << "Running count=" << count << " ...";
    if (count++ == 5) {
        invoke_timer->Cancel();
        loop->Stop();
    }
}

void NewEventLoop(struct event_base* base) {
    loop = new evpp::EventLoop(base);
    invoke_timer = loop->RunEvery(evpp::Duration(0.1), &Run);
}
}

// Test creating EventLoop from a exist event_base
TEST_UNIT(testEventLoop4) {
    struct event_base* base = event_base_new();
    auto timer = std::make_shared<evpp::TimerEventWatcher>(base, std::bind(&NewEventLoop, base), evpp::Duration(1.0));
    timer->Init();
    timer->AsyncWait();
    event_base_dispatch(base);
    event_base_free(base);
    delete loop;
    invoke_timer.reset();
    timer.reset();
    H_TEST_ASSERT(evpp::GetActiveEventCount() == 0);
}







 