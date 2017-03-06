evpp
---

# Introduction  [中文](docs/readme_cn.md)

[evpp] is a modern C++ network library for developing high performance network services in TCP/UDP/HTTP protocols.
[evpp] provides a TCP Server to support multi-threaded nonblocking event-drive server and also a HTTP, UDP Server to support http and udp prococol.

# Origin

In our business system, we need to build a TCP long-connection Gateway and other TCP services. When we do a survey of the open sources, we cannot find any appropriate network library for our demands. According to our own business characteristic, an ideal C++ network library must have the features below: 

1. A simple enough C++ interface
2. Support multi-threads and multi-processes
3. Based on[libevent] is the best. Because of the historical burden, we have some old systems which are built on [libevent]. If the ideal library is based on [libevent], we can embedded it with the old system codes, otherwhise we need to modify the old system codes or make it more complicated.

As described above, there are not many ones to choose from. So we develop one ourself. The design of the interface is highly inspired by [muduo] and [Golang]. Let's take some examples to exaplain this: 

1. `Duration` : This is a time inteval class, with a time unit. It is referenced to the implementation of `Duration` of the `Golang` project. We have seen some many cases that the time interval without a unit. For example, what does `timeout` mean?  Seconds, milliseconds or microseconds? We need to read the document carefully, even more, we need to read the implementation codes. Our `Duration` class has self-explations with the time unit.
2. `Buffer` : This is a memory buffer class. It uses the two projects' advantages of `muduo` and `Golang`.
3. `http::Server` : This is a HTTP server class with a working threads pool. It is thread-safe to dispatch tasks
4. We simply use a string with the format of `"ip:port"` to represent a network address. This is referenced to the design of `Golang`.
5. `httpc::ConnPool` : This is HTTP client connection pool with highly performance. In the future we can add more features to this class : load balance and failover.

In addition, in the implematations we pay seriously attations to thread-safe problems. An event-related resource must be initialized and released in its own `EventLoop` thread, so that we can minimize the possibility of thread-safe error. In order to achieve this goal we reimplemented `event_add` and` event_del` and other functions. Each call to `event_add`, we stored the resource into thread local storage, and in the call `event_del`, we checked it whether it is stored in the thread local storage. And then we checked all the threads local storages to see whether there are resources not destructively released when the process was exiting. The detail codes are here [https://github.com/Qihoo360/evpp /blob/master/evpp/inner_pre.cc#L46~L87](https://github.com/Qihoo360/evpp/blob/master/evpp/inner_pre.cc#L46~L87). We are so harshly pursuit the thread safety to make a program can quietly exit or reload, because we have a deep understanding of the "developing a system to run forever and developing a system to quietly exit after running a period of time are totally two diffent things".


# Features

1. Modern C++11 interface
1. Modern functional/bind style callback instead of C-style function pointer.
1. A nonblocking multi-threaded TCP server
1. A nonblocking TCP client
1. A nonblocking multi-threaded HTTP server based on the buildin http server of libevent
1. A nonblocking HTTP client
1. A nonblocking multi-threaded UDP server
1. EventLoop/Thread Pool/Timer

And also provides some libraries base on `evpp`:

1. `evmc` a nonblocking async C++ memcached (or membase cluster) client library.
2. `evnsq` a nonblocking async C++ NSQ client library. See [evnsq readme](apps/evnsq/readme.md) for details.

TODO:

1. A async redis client
2. Add `zipkin` tracing support
3. Add examples : asio chat room


# Getting Started

Please see [Quick Start](docs/quick_start.md)

# Examples

## A echo TCP server

```cpp
#include <evpp/exp.h>
#include <evpp/tcp_server.h>
#include <evpp/tcp_conn.h>
#include <evpp/buffer.h>

void OnMessage(const evpp::TCPConnPtr& conn,
               evpp::Buffer* msg,
               evpp::Timestamp ts) {
    std::string s = msg->NextAllString();
    LOG_INFO << "Received a message [" << s << "]";
    conn->Send(s);
    if (s == "quit" || s == "exit") {
        conn->Close();
    }
}

int main(int argc, char* argv[]) {
    std::string addr = std::string("0.0.0.0:9999");
    evpp::EventLoop loop;
    evpp::TCPServer server(&loop, addr, "TCPEcho", 0);
    server.SetMessageCallback(&OnMessage);
    server.Init();
    server.Start();
    loop.Run();
    return 0;
}
```

### A echo HTTP server

```cpp
#include <evpp/exp.h>
#include <evpp/http/http_server.h>

void RequestHandler(evpp::EventLoop* loop,
                    const evpp::http::ContextPtr& ctx,
                    const evpp::http::HTTPSendResponseCallback& cb) {
    cb(ctx->body.ToString());
}

int main(int argc, char* argv[]) {
    std::vector<int> ports = {9009, 23456, 23457};
    int thread_num = 2;
    evpp::http::Server server(thread_num);
    server.RegisterHandler("/echo", &RequestHandler);
    server.Init(ports);
    server.Start();
    while (!server.IsStopped()) {
        usleep(1);
    }
    return 0;
}

```


### A echo UDP server

```cpp
#include <evpp/exp.h>
#include <evpp/udp/udp_server.h>
#include <evpp/udp/udp_message.h>

void DefaultHandler(evpp::EventLoop* loop, evpp::udp::MessagePtr& msg) {
    evpp::udp::SendMessage(msg);
}

int main(int argc, char* argv[]) {
    std::vector<int> ports = {1053, 5353};
    evpp::udp::Server server;
    server.SetMessageHandler(&DefaultHandler);
    server.Init(ports);
    server.Start();

    while (!server.IsStopped()) {
        usleep(1);
    }
    return 0;
}
```

### More examples

Please see the source code in [`examples`](https://github.com/Qihoo360/evpp/tree/master/examples).

# Thanks

Thanks for the support of [Qihoo360](http://www.360.cn "http://www.360.cn").

Thanks for [libevent](https://github.com/libevent/libevent), [glog](https://github.com/google/glog), [gtest](https://github.com/google/googletest), [Golang](https://golang.org) projects.

`evpp` is highly inspired by [muduo]. Thanks for the great work of [Chen Shuo](https://github.com/chenshuo "https://github.com/chenshuo")




[evpp]:https://github.com/Qihoo360/evpp
[Golang]:https://golang.org
[muduo]:https://github.com/chenshuo/muduo
[libevent]:https://github.com/libevent/libevent