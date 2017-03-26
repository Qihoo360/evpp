#include <signal.h>
#include <evpp/exp.h>
#include <evpp/libevent_watcher.h>
#include <evpp/event_loop.h>

#include "examples/winmain-inl.h"

//int main(int argc, char* argv[]) {
//    evpp::EventLoop loop;
//    std::unique_ptr<evpp::SignalEventWatcher> ev(
//        new evpp::SignalEventWatcher(
//            SIGINT, &loop, []() { LOG_INFO << "SIGINT caught.";}));
//    ev->Init();
//    ev->AsyncWait();
//    loop.Run();
//    return 0;
//}


std::unique_ptr<evpp::SignalEventWatcher> ev;

int main(int argc, char* argv[]) {
    evpp::EventLoop loop;
    auto f1 = [&loop]() {
        ev.reset(new evpp::SignalEventWatcher(
            SIGINT, &loop, []() {
            LOG_INFO << "SIGINT caught.";
        }));
        ev->Init();
        ev->AsyncWait();
    };
    loop.RunAfter(evpp::Duration(1.0), f1);
    loop.Run();
    return 0;
}
