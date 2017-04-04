#include <evpp/event_loop.h>

#ifdef _WIN32
#include "../echo/tcpecho/winmain-inl.h"
#endif

class Printer {
public:
    Printer(evpp::EventLoop* l) : loop_(l), count_(0) {}
    void Print() {
        if (count_ < 10) {
            std::cout << "Hello, world! count=" << count_ << std::endl;
            ++count_;
        } else {
            loop_->Stop();
        }
    }

private:
    evpp::EventLoop* loop_;
    int count_;
};

int main() {
    evpp::EventLoop loop;
    Printer printer(&loop);
    loop.RunEvery(evpp::Duration(1.0), std::bind(&Printer::Print, &printer));
    loop.Run();
    return 0;
}