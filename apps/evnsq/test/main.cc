#include <evnsq/exp.h>
#include <evnsq/consumer.h>
#include <evnsq/producer.h>
#include <evpp/event_loop.h>

#include <getopt.h>

int OnMessage(const evnsq::Message* msg) {
    LOG_INFO << "Received a message, id=" << msg->id << " message=[" << std::string(msg->body, msg->body_len) << "]";
    return 0;
}

void Publish(evnsq::Producer* producer) {
    static int i = 0;
    std::stringstream ss;
    ss << "a NSQ message, index=" << i++;
    std::string msg = ss.str();
    producer->Publish("test", msg);
    LOG_INFO << "Publish : [" << msg << "]";
}

void OnReady(evpp::EventLoop* loop, evnsq::Producer* p) {
    //loop->RunEvery(evpp::Duration(1.0), std::bind(&Publish, p));
    for (int i = 0; i < 20; i++) {
        Publish(p);
    }
}


int main(int argc, char* argv[]) {
    int opt = 0;
    int digit_optind = 0;
    int option_index = 0;
    char *optstring = "t:h:";
    static struct option long_options[] = {
        { "nsqd_tcp_addr", required_argument, NULL, 't' },
        { "lookupd_http_addr", required_argument, NULL, 'h' },
        { 0, 0, 0, 0 }
    };

    std::string nsqd_tcp_addr;
    std::string lookupd_http_url;
    
    //nsqd_tcp_addr = "127.0.0.1:4150";
    nsqd_tcp_addr = "10.16.28.17:4150";
    //nsqd_tcp_addr = "weizili-L1:4150";
    //lookupd_http_url = "http://127.0.0.1:4161/lookup?topic=test";
    //lookupd_http_url = "http://10.16.28.17:4161/lookup?topic=test";

    while ((opt = getopt_long(argc, argv, optstring, long_options, &option_index)) != -1) {
        switch (opt) {
        case 't':
            nsqd_tcp_addr = optarg;
            break;
        case 'h':
            lookupd_http_url = optarg;
            break;
        default:
            printf("error argument [%s]\n", argv[optind]);
            return -1;
        }
    }

    evpp::EventLoop loop;
    evnsq::Producer client(&loop, evnsq::Option());
    client.SetMessageCallback(&OnMessage);
    client.SetReadyCallback(std::bind(&OnReady, &loop, &client));
    if (!lookupd_http_url.empty()) {
        client.ConnectToLoopupds(lookupd_http_url);
    } else {
        client.ConnectToNSQDs(nsqd_tcp_addr);
    }
    loop.Run();
    return 0;
}

#ifdef WIN32
#include "../../../examples/echo/winmain-inl.h"
#endif




