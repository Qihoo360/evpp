#include <evnsq/exp.h>
#include <evnsq/consumer.h>
#include <evnsq/producer.h>
#include <evpp/event_loop.h>

#include <chrono>
#include <thread>

#include <getopt.h>

int OnMessage(const evnsq::Message* msg) {
    LOG_INFO << "Received a message, id=" << msg->id << " message=[" << msg->body.ToString() << "]";
    return 0;
}

bool Publish(evnsq::Producer* producer) {
    static const std::string topic1 = "test1";
    static const std::string topic2 = "test2";
    static int i = 0;
    std::stringstream ss;
    ss << "a NSQ message, index=" << i++ << " ";
    std::string msg = ss.str();
    msg.append(1000, 'x');
    if (!producer->Publish(topic1, msg)) {
        return false;
    }
    //LOG_INFO << "Publish : [" << msg << "]";
    std::vector<std::string> messages;
    messages.push_back(msg);
    messages.push_back(msg);
    return producer->MultiPublish(topic2, messages);
}

void OnReady(evpp::EventLoop* loop, evnsq::Producer* p) {
    loop->RunEvery(evpp::Duration(0.001), std::bind(&Publish, p));
    for (int i = 0; i < 20; i++) {
        Publish(p);
    }
}

void Close(evnsq::Producer* p) {
    p->Close();
}

int main(int argc, char* argv[]) {
    google::InitGoogleLogging(argv[0]);

    FLAGS_stderrthreshold = 0;

    int opt = 0;
    //int digit_optind = 0;
    int option_index = 0;
    const char* optstring = "t:h:";
    static struct option long_options[] = {
        { "nsqd_tcp_addr", required_argument, NULL, 't' },
        { "lookupd_http_addr", required_argument, NULL, 'h' },
        { 0, 0, 0, 0 }
    };

    std::string nsqd_tcp_addr;
    std::string lookupd_http_url;

    nsqd_tcp_addr = "127.0.0.1:4150";
    //nsqd_tcp_addr = "weizili-L1:4150";
    //lookupd_http_url = "http://127.0.0.1:4161/lookup?topic=test";

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
        client.ConnectToLookupds(lookupd_http_url);
    } else {
        assert(nsqd_tcp_addr.size() > 0);
        client.ConnectToNSQDs(nsqd_tcp_addr);
    }

    auto f = [&loop, &client]() {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        for (;;) {
            if (loop.pending_functor_count() > 1000) {
                std::this_thread::sleep_for(std::chrono::seconds(2));
            } else {
                if (!Publish(&client)) {
                    client.Close();
                    auto quit = [&loop]() {
                        loop.Stop();
                    };
                    loop.RunAfter(evpp::Duration(2.0), quit);
                    break;
                }
            }
        }
    };
    std::thread publish_thread(f);
    loop.RunAfter(evpp::Duration(10.0), std::bind(&Close, &client));
    loop.Run();
    publish_thread.join();
    return 0;
}

#ifdef WIN32
#include "../../../examples/echo/tcpecho/winmain-inl.h"
#endif




