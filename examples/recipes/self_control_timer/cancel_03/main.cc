#include "invoke_timer.h"
#include "event_watcher.h"
#include "winmain-inl.h"

#include <event2/event.h>

void Print() {
    std::cout << __FUNCTION__ << " hello world." << std::endl;
}

void OnCanceled() {
    std::cout << __FUNCTION__ << " called." << std::endl;
}

int main() {
    struct event_base* base = event_base_new();
    auto timer1 = recipes::InvokeTimer::Create(base, 2000.0, &Print);
    timer1->set_cancel_callback(&OnCanceled);
    timer1->Start();

    auto fn = [timer1]() {
        std::cout << " cancel the timer." << std::endl;
        timer1->Cancel();
    };
    auto timer2 = recipes::InvokeTimer::Create(base, 1000.0, fn);
    timer2->Start();
    event_base_dispatch(base);
    timer1.reset();
    event_base_free(base);
    return 0;
}