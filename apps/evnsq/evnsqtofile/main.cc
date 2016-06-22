#include <evpp/exp.h>
#include <evpp/tcp_client.h>
#include <evpp/buffer.h>
#include <evpp/tcp_conn.h>

#include <evnsq/exp.h>
#include <evnsq/evnsq.h>

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
    std::string addr = "127.0.0.1:9099";
    if (argc == 2) {
        addr = argv[1];
    }
    evpp::EventLoop loop;
    evnsq::NSQClient client(&loop, addr);
//     client.SetMesageCallback(&OnMessage);
//     client.SetConnectionCallback(&OnConnection);
    client.Connect();
    loop.Run();
    return 0;
}

#ifdef WIN32
#include "../../../examples/echo/winmain-inl.h"
#endif
