#include <evpp/exp.h>
#include <evpp/tcp_server.h>
#include <evpp/event_loop_thread_pool.h>
#include <evpp/buffer.h>
#include <evpp/tcp_conn.h>

void OnMessage(const evpp::TCPConnPtr& conn,
               evpp::Buffer* msg) {
    LOG_INFO << "tid=" << std::this_thread::get_id() << " Received a message len=" << msg->size();
    if (msg->ToString() == "quit") {
        conn->Close();
        return;
    }
    conn->Send(msg->data(), msg->size());
    msg->Reset();
}

int main(int argc, char* argv[]) {
    std::string addr = "0.0.0.0:9099";
    uint32_t thread_num = 2;

    if (argc != 1 && argc != 3) {
        printf("Usage: %s <port> <thread-num>\n", argv[0]);
        printf("  e.g: %s 9099 12\n", argv[0]);
        return 0;
    }

    if (argc == 3) {
        addr = std::string("0.0.0.0:") + argv[1];
        thread_num = atoi(argv[2]);
    }

    evpp::EventLoop loop;
    evpp::EventLoopThreadPool tpool(&loop, thread_num);
    tpool.Start(true);

    std::vector<std::shared_ptr<evpp::TCPServer>> tcp_servers;
    for (uint32_t i = 0; i < thread_num; i++) {
        evpp::EventLoop* next = tpool.GetNextLoop();
        std::shared_ptr<evpp::TCPServer> s(new evpp::TCPServer(next, addr, std::to_string(i) + "#server", 0));
        s->SetMessageCallback(&OnMessage);
        s->Init();
        s->Start();
        tcp_servers.push_back(s);
    }
    
    loop.Run();
    return 0;
}

#ifdef WIN32
#include "../echo/tcpecho/winmain-inl.h"
#endif
