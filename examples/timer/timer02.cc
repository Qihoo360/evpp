#include <evpp/exp.h>
#include <evpp/event_loop.h>

#ifdef _WIN32
#include "../echo/tcpecho/winmain-inl.h"
#endif

void Print() {
    std::cout << "Hello, world!\n";
}

int main() {
    evpp::EventLoop loop;
    loop.RunEvery(evpp::Duration(1.0), &Print);
    loop.Run();
    return 0;
}