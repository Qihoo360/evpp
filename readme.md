evpp
---

# Introduction  [中文](readme_cn.md)

[evpp] is a modern C++ network library for developing high performance network services in TCP/UDP/HTTP protocols.
[evpp] provides a TCP Server to support multi-threaded nonblocking event-drive server and also a HTTP, UDP Server to support http and udp prococol.

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

1. `evmc` a nonblocking async C++ memcached (or membase cluster) client library. This library is currently used in production which sends more than 100 billion requests every day. See [evmc readme](/apps/evmc/readme.md) for more details.
2. `evnsq` a nonblocking async C++ NSQ client library. This library is currently used in production which processes more than 20 billion messages every day. See [evnsq readme](apps/evnsq/readme.md) for more details.


NOTE: master is our development branch and may not be stable at all times.

# Origin

In our business system, we need to build a TCP long-connection Gateway and other TCP services. When we do a survey of the open sources, we cannot find any appropriate network library for our demands. According to our own business characteristic, an ideal C++ network library must have the features below: 

1. A simple enough C++ interface
2. Support multi-threads and multi-processes
3. Based on [libevent] is preferable for our projects. Given your older applications were based on [libevent], it was preferable to have your new framework also be based on it, so as to reduce the overall time/effort/cost to completion. Actually, we do have some older applications which were based on [libevent].

As described above, there are not many options to choose from. So we developed one ourself. The design of the interface is highly inspired by [muduo] and [Golang]. Let's take some examples to exaplain this: 

1. `Duration` : This is a time inteval class, with a time unit. It is referenced to the implementation of `Duration` of the [Golang] project. We have seen some many cases that the time interval without a unit. For example, what does `timeout` mean?  Seconds, milliseconds or microseconds? We need to read the document carefully, even more, we need to read the implementation codes. Our `Duration` class has self-explations with the time unit. Additionally `std::chrono::duration` in the STL of C++11 has the similar implementations, but it is a little bit complicated.
2. `Buffer` : This is a memory buffer class. It uses the advantages of the two projects [muduo] and [Golang].
3. `http::Server` : This is a HTTP server class with a working threads pool. It is thread-safe to dispatch tasks
4. We simply use a string with the format of `"ip:port"` to represent a network address. This is referenced to the design of [Golang].
5. `httpc::ConnPool` : This is HTTP client connection pool with highly performance. In the future we can add more features to this class : load balance and failover.

In addition, in the implematations we pay seriously attations to thread-safe problems. An event-related resource must be initialized and released in its own `EventLoop` thread, so that we can minimize the possibility of thread-safe error. In order to achieve this goal we reimplemented `event_add` and` event_del` and other functions. Each call to `event_add`, we stored the resource into thread local storage, and in the call `event_del`, we checked it whether it is stored in the thread local storage. And then we checked all the threads local storages to see whether there are resources not destructively released when the process was exiting. The detail codes are here [https://github.com/Qihoo360/evpp/blob/master/evpp/inner_pre.cc#L46~L87](https://github.com/Qihoo360/evpp/blob/master/evpp/inner_pre.cc#L46~L87). We are so harshly pursuit the thread safety to make a program can quietly exit or reload, because we have a deep understanding of the "developing a system to run forever and developing a system to quietly exit after running a period of time are totally two diffent things".


# Getting Started

Please see [Quick Start](docs/quick_start.md)

# Benchmark

### Throughput

The throughput benchmark of [evpp] is *40%* higher than *`boost.asio`* and *17%* higher than [libevent].
The reason of [evpp] has a better throughput benchmark than [libevent] is that [evpp] implements its own IO buffer instead of [libevent]'s evbuffer. 

The benchmark code is here [https://github.com/Qihoo360/evpp/tree/master/benchmark/](https://github.com/Qihoo360/evpp/tree/master/benchmark/).


##### Test objects:

1. [evpp-0.2.0](https://github.com/Qihoo360/evpp/archive/0.2.0.tar.gz) based on libevent-2.0.21
2. boost.asio-1.53
3. libevent-2.0.21

##### Environment

- Linux CentOS 6.2, 2.6.32-220.7.1.el6.x86_64
- Intel(R) Xeon(R) CPU E5-2630 v2 @ 2.60GHz

![](docs/benchmark/throughput-1thread-1024bytes.png)
![](docs/benchmark/throughput-1thread-2048bytes.png)
![](docs/benchmark/throughput-1thread-4096bytes.png)
![](docs/benchmark/throughput-1thread-8192bytes.png)
![](docs/benchmark/throughput-multi-thread-4096bytes-evpp-vs-asio.png)

### More Benchmarks

[The throughput benchmark of evpp VS muduo](docks/benchmark_throughput_vs_muduo_cn.md)

# Examples

## An echo TCP server

```cpp
#include <evpp/tcp_server.h>
#include <evpp/buffer.h>
#include <evpp/tcp_conn.h>

int main(int argc, char* argv[]) {
    std::string addr = "0.0.0.0:9099";
    int thread_num = 4;
    evpp::EventLoop loop;
    evpp::TCPServer server(&loop, addr, "TCPEchoServer", thread_num);
    server.SetMessageCallback([](const evpp::TCPConnPtr& conn,
                                 evpp::Buffer* msg,
                                 evpp::Timestamp ts) {
        conn->Send(msg);
    });
    server.SetConnectionCallback([](const evpp::TCPConnPtr& conn) {
        if (conn->IsConnected()) {
            LOG_INFO << "A new connection from " << conn->remote_addr();
        } else {
            LOG_INFO << "Lost the connection from " << conn->remote_addr();
        }
    });
    server.Init();
    server.Start();
    loop.Run();
    return 0;
}
```

### An echo HTTP server

```cpp
#include <evpp/exp.h>
#include <evpp/http/http_server.h>

int main(int argc, char* argv[]) {
    std::vector<int> ports = { 9009, 23456, 23457 };
    int thread_num = 2;
    evpp::http::Server server(thread_num);
    server.RegisterHandler("/echo",
                           [](evpp::EventLoop* loop,
                              const evpp::http::ContextPtr& ctx,
                              const evpp::http::HTTPSendResponseCallback& cb) {
        cb(ctx->body().ToString()); }
    );
    server.Init(ports);
    server.Start();
    while (!server.IsStopped()) {
        usleep(1);
    }
    return 0;
}

```


### An echo UDP server

```cpp
#include <evpp/exp.h>
#include <evpp/udp/udp_server.h>
#include <evpp/udp/udp_message.h>

int main(int argc, char* argv[]) {
    std::vector<int> ports = { 1053, 5353 };
    evpp::udp::Server server;
    server.SetMessageHandler([](evpp::EventLoop* loop, evpp::udp::MessagePtr& msg) {
        evpp::udp::SendMessage(msg);
    });
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

# TODO

1. An async redis client
2. Add `zipkin` tracing support
3. Add examples : asio chat room
4. Fix the comments written in mandarin problem
5. Add benchmark against with boost.asio/cpp-netlib/beast http library/muduo/libevent/libuv ... 

# In Production

<img src="http://i.imgur.com/dcHpCm4.png" height = "100" width = "120" alt="Qihoo">

> Welcome to join this list :-)


# Thanks

Thanks for the support of [Qihoo360](http://www.360.cn "http://www.360.cn").

Thanks for [libevent], [glog], [gtest], [Golang], [LevelDB], [rapidjson] projects.

[evpp] is highly inspired by [muduo]. Thanks for the great work of [Chen Shuo](https://github.com/chenshuo "https://github.com/chenshuo")


[gtest]:https://github.com/google/googletest
[glog]:https://github.com/google/glog
[evpp]:https://github.com/Qihoo360/evpp
[Golang]:https://golang.org
[muduo]:https://github.com/chenshuo/muduo
[libevent]:https://github.com/libevent/libevent
[LevelDB]:https://github.com/google/leveldb
[rapidjson]:https://github.com/miloyip/
