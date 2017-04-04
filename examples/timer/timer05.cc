#include <evpp/event_loop.h>

#ifdef _WIN32
#include "../echo/tcpecho/winmain-inl.h"
#endif

class Printer {
public:
    Printer(evpp::EventLoop* l) : loop_(l), count_(0) {}
    void Print1() {
        std::cout << "Print1 : count=" << count_ << std::endl;
        ++count_;
    }

    void Print2() {
        std::cout << "Print2 : count=" << count_ << std::endl;
        ++count_;
    }
private:
    evpp::EventLoop* loop_;
    int count_;
};

int main() {
    evpp::EventLoop loop;
    Printer printer(&loop);
    loop.RunEvery(evpp::Duration(1.0), std::bind(&Printer::Print1, &printer));
    loop.RunEvery(evpp::Duration(2.0), std::bind(&Printer::Print2, &printer));
    auto f = [&loop]() {
        loop.Stop();
    };
    loop.RunAfter(evpp::Duration(10.0), f);
    loop.Run();
    return 0;
}