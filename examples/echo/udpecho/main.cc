#include <evpp/exp.h>
#include <evpp/udp/udp_server.h>
#include <evpp/udp/udp_message.h>
#include <evpp/event_loop.h>
#include <evpp/event_loop_thread_pool.h>

#ifdef _WIN32
#include "../tcpecho/winmain-inl.h"
#endif

void DefaultHandler(evpp::EventLoop* loop, evpp::udp::MessagePtr& msg) {
    std::stringstream oss;
    oss << "func=" << __FUNCTION__ << " OK"
        << " body=" << msg->NextAllString() << "\n";
    evpp::udp::SendMessage(msg);
}

int main(int argc, char* argv[]) {
    std::vector<int> ports = {1053, 5353};
    int port = 29099;
    int thread_num = 2;

    if (argc > 1) {
        if (std::string("-h") == argv[1] ||
            std::string("--h") == argv[1] ||
            std::string("-help") == argv[1] ||
            std::string("--help") == argv[1]) {
            std::cout << "usage : " << argv[0] << " <listen_port> <thread_num>\n";
            std::cout << " e.g. : " << argv[0] << " 8080 24\n";
            return 0;
        }
    }

    if (argc == 2) {
        port = atoi(argv[1]);
    }
    ports.push_back(port);

    if (argc == 3) {
        port = atoi(argv[1]);
        thread_num = atoi(argv[2]);
    }

    evpp::udp::Server server;
    server.SetThreadDispatchPolicy(evpp::ThreadDispatchPolicy::kIPAddressHashing);
    server.SetMessageHandler(&DefaultHandler);
    server.Start(ports);

    evpp::EventLoop base_loop;
    std::shared_ptr<evpp::EventLoopThreadPool> tpool(new evpp::EventLoopThreadPool(&base_loop, thread_num));
    tpool->Start(true);
    server.SetEventLoopThreadPool(tpool);
    base_loop.Run();
    return 0;
}
