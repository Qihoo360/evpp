#include <evpp/event_loop.h>

#ifdef _WIN32
#include "../echo/tcpecho/winmain-inl.h"
#endif

void Print(evpp::EventLoop* loop, int* count) {
    if (*count < 3) {
        std::cout << "Hello, world! count=" << *count << std::endl;
        ++(*count);
        loop->RunAfter(evpp::Duration(2.0), std::bind(&Print, loop, count));
    } else {
        loop->Stop();
    }
}

int main() {
    evpp::EventLoop loop;
    int count = 0;
    loop.RunAfter(evpp::Duration(2.0), std::bind(&Print, &loop, &count));
    loop.Run();
    return 0;
}