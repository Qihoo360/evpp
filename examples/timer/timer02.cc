#include <evpp/event_loop.h>

#ifdef _WIN32
#include "../echo/tcpecho/winmain-inl.h"
#endif

int main() {
    evpp::EventLoop loop;
    loop.RunEvery(evpp::Duration(1.0), []() { std::cout << "Hello, world!\n"; });
    loop.Run();
    return 0;
}