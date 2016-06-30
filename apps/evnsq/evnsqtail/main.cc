#include <evpp/exp.h>
#include <evpp/tcp_client.h>
#include <evpp/buffer.h>
#include <evpp/tcp_conn.h>

#include <evnsq/exp.h>
#include <evnsq/consumer.h>



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
    std::string nsqd_tcp_addr;
    std::string lookupd_http_url;
    //nsqd_tcp_addr = "127.0.0.1:4150";
    //nsqd_tcp_addr = "10.16.28.17:4150";
    //nsqd_tcp_addr = "weizili-L1:4150";
    lookupd_http_url = "http://127.0.0.1:4161/lookup?topic=test";
    if (argc == 2) {
        if (strncmp(argv[1], "http", 4) == 0) {
            lookupd_http_url = argv[1];
        } else {
            nsqd_tcp_addr = argv[1];
        }
    }
    evpp::EventLoop loop;
    evnsq::Consumer client(&loop, "test", "ch1", evnsq::Option());
    client.SetMessageCallback(&OnMessage);
    if (!nsqd_tcp_addr.empty()) {
        client.ConnectToNSQDs(nsqd_tcp_addr);
    } else {
        client.ConnectToLoopupds(lookupd_http_url);
    }
    loop.Run();
    return 0;
}

#ifdef WIN32
#include "../../../examples/echo/winmain-inl.h"
#endif
