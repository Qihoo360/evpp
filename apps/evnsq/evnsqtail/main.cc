#include <evpp/exp.h>
#include <evpp/tcp_client.h>
#include <evpp/buffer.h>
#include <evpp/tcp_conn.h>

#include <evnsq/exp.h>

#include <evnsq/evnsq.h>



int OnMessage(const evnsq::Message* msg) {
    LOG_INFO << "Received a message, id=" << msg->id << " message=[" << std::string(msg->body, msg->body_len) << "]";
    return 0;
}

void OnConnection(const evpp::TCPConnPtr& conn) {
    if (conn->IsConnected()) {
        conn->Send("hello");
    } else {
        LOG_INFO << "Disconnected from " << conn->remote_addr();
    }
}

int main(int argc, char* argv[]) {
    std::string addr = "127.0.0.1:4150";
    //std::string  addr = "10.16.28.17:4150";
    if (argc == 2) {
        addr = argv[1];
    }
    evpp::EventLoop loop;
    evnsq::Consumer client(&loop, "test", "ch1", evnsq::Option());
    client.SetMessageCallback(&OnMessage);
//     client.SetConnectionCallback(&OnConnection);
    client.ConnectToNSQDs(addr);
    loop.Run();
    return 0;
}

#ifdef WIN32
#include "../../../examples/echo/winmain-inl.h"
#endif
