#include <signal.h>
#include <evpp/event_watcher.h>
#include <evpp/event_loop.h>

#include "examples/winmain-inl.h"


int main(int argc, char* argv[]) {
    evpp::EventLoop loop;
    std::unique_ptr<evpp::SignalEventWatcher> ev;
    auto f = [&ev, &loop]() {
        ev.reset(new evpp::SignalEventWatcher(
            SIGINT, &loop, []() {
            LOG_INFO << "SIGINT caught.";
        }));
        ev->Init();
        ev->AsyncWait();
    };
    loop.RunAfter(evpp::Duration(0.001), f);
    loop.Run();
    return 0;
}
