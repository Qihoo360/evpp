#include <evpp/tcp_client.h>
#include <evpp/buffer.h>
#include <evpp/tcp_conn.h>

int main(int argc, char* argv[]) {
    std::string addr = "127.0.0.1:9099";

    if (argc == 2) {
        addr = argv[1];
    }

    evpp::EventLoop loop;
    evpp::TCPClient client(&loop, addr, "TCPPingPongClient");
    client.SetMessageCallback([&loop, &client](const evpp::TCPConnPtr& conn,
                               evpp::Buffer* msg) {
        LOG_TRACE << "Receive a message [" << msg->ToString() << "]";
        client.Disconnect();
        loop.RunAfter(500.0, [&loop]() { loop.Stop(); });
    });

    client.SetConnectionCallback([](const evpp::TCPConnPtr& conn) {
        if (conn->IsConnected()) {
            LOG_INFO << "Connected to " << conn->remote_addr();
            conn->Send("hello");
        } else {
            LOG_INFO << "Disconnected from " << conn->remote_addr();
        }
    });
    client.Connect();
    loop.Run();
    return 0;
}

#include "../echo/tcpecho/winmain-inl.h"