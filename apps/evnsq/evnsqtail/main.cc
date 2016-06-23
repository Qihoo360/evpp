#include <evpp/exp.h>
#include <evpp/tcp_client.h>
#include <evpp/buffer.h>
#include <evpp/tcp_conn.h>

#include <evnsq/exp.h>

#include <evnsq/evnsq.h>



void OnMessage(const evnsq::Message* msg) {
    LOG_INFO << "Received a message, id=" << std::string(msg->id, evnsq::kMessageIDLen) << " message=[" << std::string(msg->body, msg->body_len) << "]";
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
    if (argc == 2) {
        addr = argv[1];
    }
    evpp::EventLoop loop;
    evnsq::Consumer client(&loop, "test", "ch1", evnsq::Option());
    client.SetMessageCallback(&OnMessage);
//     client.SetConnectionCallback(&OnConnection);
    client.ConnectToNSQD(addr);
    loop.Run();
    return 0;
}

#ifdef WIN32
#include "../../../examples/echo/winmain-inl.h"
#endif
