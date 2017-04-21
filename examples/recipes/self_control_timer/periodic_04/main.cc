#include "invoke_timer.h"
#include "event_watcher.h"
#include "winmain-inl.h"

#include <event2/event.h>

void Print() {
    std::cout << __FUNCTION__ << " hello world." << std::endl;
}

int main() {
    struct event_base* base = event_base_new();
    auto timer = recipes::InvokeTimer::Create(base, 1000.0, &Print, true);
    timer->Start();
    timer.reset();
    event_base_dispatch(base);
    event_base_free(base);
    return 0;
}