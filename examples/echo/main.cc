#include <evpp/exp.h>
#include <evpp/tcp_server.h>
#include <evpp/buffer.h>
#include <evpp/tcp_conn.h>

void OnMessage(const evpp::TCPConnPtr& conn,
               evpp::Buffer* msg,
               evpp::Timestamp ts) {
    std::string s = msg->NextAllString();
    LOG_INFO << "Received a message [" << s << "]";
    conn->Send(s);

    if (s == "quit" || s == "exit") {
        conn->Close();
    }
}

int main(int argc, char* argv[]) {
    std::string port = "9099";

    if (argc == 2) {
        port = argv[1];
    }

    std::string addr = std::string("0.0.0.0:") + port;
    evpp::EventLoop loop;
    evpp::TCPServer server(&loop, addr, "TCPEcho", 0);
    server.SetMessageCallback(&OnMessage);
    server.Start();
    loop.Run();
    return 0;
}

#ifdef WIN32
#include "winmain-inl.h"
#endif
