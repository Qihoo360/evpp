evnsq
---

`evnsq` is a nonblocking async C++ client library for [NSQ](https://github.com/nsqio/nsq). It is based on [evpp](https://github.com/Qihoo360/evpp) which is a modern C++ network library.

### Status

This library is currently used in production which processes more than 20 billion messages every day.


### Features

1. Support single `NSQD` instance
2. Support a cluster of `NSQDs`
3. Support `nsqlookupd`
4. Support tow modes : producer and consumer
5. Support AUTH
6. Support failover and load balance

### A consumer : tailer

```C++
#include <evnsq/exp.h>
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

    if (!lookupd_http_url.empty()) {
        client.ConnectToLoopupds(lookupd_http_url);
    } else {
        client.ConnectToNSQDs(nsqd_tcp_addr);
    }

    loop.Run();
    return 0;
}
```

### A producer example

```C++
#include <evnsq/exp.h>
#include <evnsq/consumer.h>
#include <evnsq/producer.h>
#include <evpp/event_loop.h>

#include <chrono>
#include <thread>

#include <getopt.h>

void Publish(evnsq::Producer* producer) {
    static const std::string topic1 = "test1";
    static const std::string topic2 = "test2";
    static int i = 0;
    std::stringstream ss;
    ss << "a NSQ message, index=" << i++ << " ";
    std::string msg = ss.str();
    msg.append(1000, 'x');
    producer->Publish(topic1, msg);
    //LOG_INFO << "Publish : [" << msg << "]";
    std::vector<std::string> messages;
    messages.push_back(msg);
    messages.push_back(msg);
    producer->MultiPublish(topic2, messages);
}

void OnReady(evpp::EventLoop* loop, evnsq::Producer* p) {
    loop->RunEvery(evpp::Duration(0.001), std::bind(&Publish, p));
    for (int i = 0; i < 20; i++) {
        Publish(p);
    }
}


int main(int argc, char* argv[]) {
    google::InitGoogleLogging(argv[0]);

    FLAGS_stderrthreshold=0;

    int opt = 0;
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
    lookupd_http_url = "http://127.0.0.1:14561/nodes";

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
    client.SetReadyCallback(std::bind(&OnReady, &loop, &client));

    if (!lookupd_http_url.empty()) {
        client.ConnectToLoopupds(lookupd_http_url);
    } else {
        client.ConnectToNSQDs(nsqd_tcp_addr);
    }

    auto f = [](evpp::EventLoop* l, evnsq::Producer* c) {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        for (;;) {
            if (l->pending_functor_count() > 10000) {
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
            } else {
                Publish(c);
            }
        }
    };
    std::thread publish_thread(std::bind(f, &loop, &client));
    loop.Run();
    return 0;
}

```

### Dependencies

- [evpp](https://github.com/nsqio/nsq)
- [rapidjson](https://github.com/nsqio/nsq)
- [libevent](https://github.com/libevent/libevent)
- [glog](https://github.com/google/glog)
