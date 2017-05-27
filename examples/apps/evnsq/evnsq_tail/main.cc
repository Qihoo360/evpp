#include <evnsq/consumer.h>
#include <evpp/event_loop.h>


int OnMessage(const evnsq::Message* msg) {
    LOG_INFO << "Received a message, id=" << msg->id << " message=[" << msg->body.ToString() << "]";
    return 0;
}

int main(int argc, char* argv[]) {
    std::string nsqd_tcp_addr;
    std::string lookupd_http_url;
    nsqd_tcp_addr = "127.0.0.1:4150";
    //nsqd_tcp_addr = "weizili-L1:4150";
    //lookupd_http_url = "http://127.0.0.1:4161/lookup?topic=test";

    if (argc == 2) {
        if (strncmp(argv[1], "http", 4) == 0) {
            lookupd_http_url = argv[1];
        } else {
            nsqd_tcp_addr = argv[1];
        }
    }

    evpp::EventLoop loop;
    evnsq::Consumer client(&loop, "test1", "ch1", evnsq::Option());
    client.SetMessageCallback(&OnMessage);

    if (!lookupd_http_url.empty()) {
        client.ConnectToLookupds(lookupd_http_url);
    } else {
        client.ConnectToNSQDs(nsqd_tcp_addr);
    }

    loop.Run();
    return 0;
}

#ifdef WIN32
#include "../../../echo/tcpecho/winmain-inl.h"
#endif
