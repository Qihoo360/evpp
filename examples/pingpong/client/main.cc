#include <evpp/exp.h>
#include <evpp/tcp_client.h>
#include <evpp/buffer.h>
#include <evpp/tcp_conn.h>

std::string message(128, 'x');

void OnMessage(const evpp::TCPConnPtr& conn,
               evpp::Buffer* msg,
               evpp::Timestamp ts) {
    std::string s = msg->NextAllString();
    LOG_INFO << "Received a message size=" << s.size() << " [" << s << "]";
    conn->Send(s.data(), s.size());
    //usleep(1*1000000);
}

void OnConnection(const evpp::TCPConnPtr& conn) {
    if (conn->IsConnected()) {
        conn->Send(message);
    } else {
        LOG_INFO << "Disconnected from " << conn->remote_addr();
    }
}

int main(int argc, char* argv[]) {
    //std::string addr = "10.16.29.131:9099";
    std::string addr = "build15v.kill.corp.qihoo.net:9099";
    if (argc == 2) {
        addr = argv[1];
    }
    evpp::EventLoop loop;
    evpp::TCPClient client(&loop, addr, "TCPPingPongClient");
    client.SetMessageCallback(&OnMessage);
    client.SetConnectionCallback(&OnConnection);
    client.Connect();
    loop.Run();
    return 0;
}

#ifdef WIN32
#include "../../echo/winmain-inl.h"
#endif
