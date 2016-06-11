#include <evpp/exp.h>
#include <evpp/tcp_client.h>
#include <evpp/buffer.h>
#include <evpp/tcp_conn.h>

void OnMessage(const evpp::TCPConnPtr& conn,
               evpp::Buffer* msg,
               evpp::Timestamp ts) {
    std::string s = msg->NextAllString();
    LOG_INFO << "Received a message [" << s << "]";
    conn->Send(s.data(), s.size());
    usleep(1*1000000);
}

void OnConnection(const evpp::TCPConnPtr& conn) {
    if (conn->IsConnected()) {
        conn->Send("hello");
    } else {
        LOG_INFO << "Disconnected from " << conn->remote_addr();
    }
}

int main(int argc, char* argv[]) {
    std::string addr = "10.16.29.131:9099";
    if (argc == 2) {
        addr = argv[1];
    }
    evpp::EventLoop loop;
    evpp::TCPClient client(&loop, addr, "TCPPingPongClient");
    client.SetMesageHandler(&OnMessage);
    client.SetConnectionCallback(&OnConnection);
    client.Connect();
    loop.Run();
    return 0;
}

#ifdef WIN32
#include "winmain-inl.h"
#endif
