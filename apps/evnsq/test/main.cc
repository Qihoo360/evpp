#include <evnsq/exp.h>
#include <evnsq/consumer.h>
#include <evnsq/producer.h>
#include <evpp/event_loop.h>

int OnMessage(const evnsq::Message* msg) {
    LOG_INFO << "Received a message, id=" << msg->id << " message=[" << std::string(msg->body, msg->body_len) << "]";
    return 0;
}

void Publish(evnsq::Producer* producer) {
    static int i = 0;
    std::stringstream ss;
    ss << "a NSQ message, index=" << i++;
    std::string msg = ss.str();
    producer->Publish(msg);
    LOG_INFO << "Publish : [" << msg << "]";
}

void OnReady(evpp::EventLoop* loop, evnsq::Producer* p) {
    loop->RunEvery(evpp::Duration(1.0), std::bind(&Publish, p));
}


int main(int argc, char* argv[]) {
    std::string nsqd_tcp_addr;
    std::string lookupd_http_url;
    //nsqd_tcp_addr = "127.0.0.1:4150";
    //nsqd_tcp_addr = "10.16.28.17:4150";
    //nsqd_tcp_addr = "weizili-L1:4150";
    //lookupd_http_url = "http://127.0.0.1:4161/lookup?topic=test";
    lookupd_http_url = "http://10.16.28.17:4161/lookup?topic=test";
    if (argc == 2) {
        if (strncmp(argv[1], "http", 4) == 0) {
            lookupd_http_url = argv[1];
        } else {
            nsqd_tcp_addr = argv[1];
        }
    }

    evpp::EventLoop loop;
    evnsq::Producer client(&loop, "test", "ch1", evnsq::Option());
    client.SetMessageCallback(&OnMessage);
    client.SetReadyCallback(std::bind(&OnReady, &loop, &client));
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
