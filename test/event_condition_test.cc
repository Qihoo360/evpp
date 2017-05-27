
#include "test_common.h"

#include <evpp/libevent.h>
#include <evpp/event_watcher.h>
#include <evpp/event_loop.h>
#include <thread>

// namespace {
// static bool g_event_handler_called = false;
// static void Handle(struct event_base* base) {
//     g_event_handler_called = true;
//     event_base_loopexit(base, 0);
// }
// 
// static void MyEventThread(struct event_base* base, evpp::PipeEventWatcher* ev) {
//     if (ev->Init()) {
//         ev->AsyncWait();
//     }
// 
//     event_base_loop(base, 0);
//     delete ev;// make sure to initialize and delete in the same thread.
// }
// }
// 
// TEST_UNIT(testPipeEventWatcher) {
//     struct event_base* base = event_base_new();
//     evpp::PipeEventWatcher* ev = new evpp::PipeEventWatcher(base, std::bind(&Handle, base));
//     std::thread th(MyEventThread, base, ev);
//     ::usleep(1000 * 100);
//     ev->Notify();
//     th.join();
//     event_base_free(base);
//     H_TEST_ASSERT(g_event_handler_called == true);
//     H_TEST_ASSERT(evpp::GetActiveEventCount() == 0);
// }

namespace {
static bool g_event_handler_called = false;
static void Handle(evpp::EventLoop* loop) {
    g_event_handler_called = true;
    loop->Stop();
}

static void MyEventThread(evpp::EventLoop* loop, evpp::PipeEventWatcher* ev) {
    if (ev->Init()) {
        ev->AsyncWait();
    }

    loop->Run();
    delete ev; // make sure to initialize and delete in the same thread.
}
}

TEST_UNIT(testPipeEventWatcher) {
    std::unique_ptr<evpp::EventLoop> loop(new evpp::EventLoop);
    evpp::PipeEventWatcher* ev = new evpp::PipeEventWatcher(loop.get(), std::bind(&Handle, loop.get()));
    std::thread th(MyEventThread, loop.get(), ev);
    ::usleep(1000 * 100);
    ev->Notify();
    th.join();
    loop.reset();
    H_TEST_ASSERT(g_event_handler_called == true);
    H_TEST_ASSERT(evpp::GetActiveEventCount() == 0);
}



