#include <evnsq/exp.h>
#include <evnsq/consumer.h>
#include <evnsq/producer.h>
#include <evpp/event_loop.h>

#include <chrono>
#include <thread>

#include <getopt.h>

size_t total_count = 200;

bool Publish(evnsq::Producer* producer) {
    LOG_INFO << "Publish(evnsq::Producer* producer) published_count=" << producer->published_count();
    if (producer->published_count() == total_count) {
        producer->Close();
        auto loop = producer->loop();
        evpp::InvokeTimerPtr timer = evpp::any_cast<evpp::InvokeTimerPtr>(loop->context());
        loop->set_context(evpp::Any());
        auto quit = [loop]() {
            loop->Stop();
        };
        timer->set_cancel_callback(quit);
        timer->Cancel();
        return false;
    }
    static const std::string topic1 = "test1";
    static int i = 0;
    std::stringstream ss;
    ss << "a NSQ message, index=" << i++ << " ";
    std::string msg = ss.str();
    msg.append(1000, 'x');
    return producer->Publish(topic1, msg);
}

void OnReady(evpp::EventLoop* loop, evnsq::Producer* p) {
    if (loop->context().IsEmpty()) {
        evpp::InvokeTimerPtr timer = loop->RunEvery(evpp::Duration(0.1), std::bind(&Publish, p));
        loop->set_context(evpp::Any(timer));
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
    const char* optstring = "t:h:s:c:";
    static struct option long_options[] = {
        { "nsqd_tcp_addr", required_argument, NULL, 't' },
        { "lookupd_http_addr", required_argument, NULL, 'h' },
        { "auth_secret", required_argument, NULL, 's' },
        { "total_count", required_argument, NULL, 'c' },
        { 0, 0, 0, 0 }
    };

    std::string nsqd_tcp_addr;
    std::string lookupd_http_url;
    std::string auth_secret;

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

        case 's':
            auth_secret = optarg;
            break;

        case 'c':
            total_count = size_t(std::atoi(optarg));
            break;

        default:
            printf("error argument [%s]\n", argv[optind]);
            return -1;
        }
    }

    evpp::EventLoop loop;
    evnsq::Option op;
    op.auth_secret = auth_secret;
    evnsq::Producer client(&loop, op);
    client.SetReadyCallback(std::bind(&OnReady, &loop, &client));

    auto cleanup = [&loop, &client]() {
        client.Close();
        auto quit = [&loop]() {
            loop.Stop();
        };
        loop.RunAfter(evpp::Duration(2.0), quit);
    };

    client.SetCloseCallback(cleanup);


    if (!lookupd_http_url.empty()) {
        client.ConnectToLookupds(lookupd_http_url);
    } else {
        assert(nsqd_tcp_addr.size() > 0);
        client.ConnectToNSQDs(nsqd_tcp_addr);
    }

    loop.Run();
    return 0;
}

#ifdef WIN32
#include "../../../../examples/echo/tcpecho/winmain-inl.h"
#endif




