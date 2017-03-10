evpp
---

# 简介 [English](readme.md)

[evpp]是一个基于[libevent]开发的现代化C++11高性能网络服务器，自带TCP/UDP/HTTP等协议的异步非阻塞式的服务器和客户端库。


# 特性

1. 现代版的C++11接口
1. 非阻塞异步接口都是C++11的functional/bind形式的回调仿函数（不是[libevent]中的C风格的函数指针）
1. 非阻塞纯异步多线程TCP服务器/客户端
1. 非阻塞纯异步多线程HTTP服务器/客户端
1. 非阻塞纯异步多线程UDP服务器
1. 支持多进程模式
1. 优秀的跨平台特性和高性能（继承自[libevent]的优点）

除此之外，基于该库之上，还提供两个附带的应用层协议库：

1. `evmc` ：一个纯异步非阻塞式的`memcached`的C++客户端库，支持`membase`集群模式。该库已经用于生产环境，每天发起1000+亿条memcache查询请求。详情请见：[evmc readme](/apps/evmc/readme.md)
2. `evnsq` ： 一个纯异步非阻塞式的`NSQ`的C++客户端库，支持消费者、生产者、服务发现等特性。该库已经用于生产环境，每天生产200+亿条日志消息。详情请见：[evnsq readme](/apps/evnsq/readme.md)

将来还会推出`redis`的客户端库。


# 项目由来

我们开发小组负责的业务需要用到TCP协议来建设长连接网关服务和一些其他的一些基于TCP的短连接服务，在调研开源项目的过程中，没有发现一个合适的库来满足我们要求。结合我们自身的业务情况，理想中的C++网络库应具备一下几个特性：

1. 接口简单易用，最好是C++接口
2. 多线程，也能支持多进程
2. 最好是基于[libevent]实现（因为现有的历史遗留框架、基础库等是依赖[libevent]），这样能很方便嵌入[libevent]的事件循环，否则改动较大或者集成起来的程序可能会有很多跨线程的调用

基于这些需要，可供选择的不多，所以我们只能自己开发一个。开发过程中，接口设计方面基本上大部分是参考[muduo]项目来设计和实现的，当然也做了一些取舍和增改；同时也大量借鉴了[Golang]的一些设计哲学和思想，举几个小例子来说明一下：

1. `Duration` ： 这是一个时间区间相关的类，自带时间单位信息，参考了[Golang]项目中的`Duration`实现。我们在其他项目中见到太多的时间是不带单位的，例如`timeout`，到底是秒、毫秒还是微秒？需要看文档说明或具体实现，好一点的设计会将单位带在变量名中，例如`timeout_ms`，但还是没有`Duration`这种独立的类好。目前C++11中也有类似的实现`std::chrono::duration`，但稍显复杂，没有咱们这个借鉴[Golang]实现的版本来的简单明了。
2. `Buffer` ： 这是一个缓冲区类，融合了`muduo`和[Golang]两个项目中相关类的设计和实现
3. `http::Server` : 这是一个http服务器类，自带线程池，它的事件循环和工作线程调度，完全是线程安全的，业务层不用太多关心跨线程调用问题。同时，还将http服务器的核心功能单独抽取出来形成`http::Service`类，是一个可嵌入型的服务器类，可以嵌入到已有的[libevent]事件循环中。
4. 网络地址的表达就仅仅使用`"ip:port"`这种形式字符串表示，就是参考[Golang]的设计
5. `httpc::ConnPool`是一个http的客户端连接池库，设计上尽量考虑高性能和复用。以后基于此还可以增加负载均衡和故障转移等特性。

另外，我们实现过程中极其重视线程安全问题，一个事件相关的资源必须在其所属的`EventLoop`中初始化和析构释放，这样我们能最大限度的减少出错的可能。为了达到这个目标我们重载`event_add`和`event_del`等函数，每一次调用`event_add`，就在对应的线程私有数据中记录该对象，在调用`event_del`时，检查之前该线程私有数据中是否拥有该对象，然后在整个程序退出前，再完整的检查所有线程的私有数据，看看是否仍然有对象没有析构释放，详细代码实现可以参考 [https://github.com/Qihoo360/evpp/blob/master/evpp/inner_pre.cc#L46~L87](https://github.com/Qihoo360/evpp/blob/master/evpp/inner_pre.cc#L46~L87)。我们如此苛刻的追求线程安全，只是为了让一个程序能安静的平稳的退出或Reload，因为我们深刻的理解“编写永远运行的系统，和编写运行一段时间后平静关闭的系统是两码事”，后者要困难的多。


# 快速开始

请见 [Quick Start](quick_start.md)


# Benchmark

### 吞吐量测试

本文用 ping pong 测试来对比[evpp]与[libevent]、*`boost.asio`* 等网络的吞吐量，测试结果表明[evpp]吞吐量平均比 *`boost.asio`*  高 *40%* 左右，比[libevent]高17%z左右。

[evpp]本身是基于[libevent]实现的，不过[evpp]只是用了[libevent]的事件循环，并没有用[libevent]的`evbuffer`，而是自己参考[muduo]和[Golang]实现了自己的网络IO读写类[Buffer](https://github.com/Qihoo360/evpp/blob/master/evpp/buffer.h)。

性能测试代码在这里：[https://github.com/Qihoo360/evpp/tree/master/benchmark/tcp](https://github.com/Qihoo360/evpp/tree/master/benchmark/tcp).

##### 测试对象

1. [evpp-0.2.0](https://github.com/Qihoo360/evpp/archive/0.2.0.tar.gz) based on libevent-2.0.21
2. boost.asio-1.53
3. libevent-2.0.21

##### 系统环境

- 操作系统：Linux CentOS 6.2, 2.6.32-220.7.1.el6.x86_64
- 硬件CPU：Intel(R) Xeon(R) CPU E5-2630 v2 @ 2.60GHz

![](docs/benchmark/throughput-1thread-1024bytes.png)
![](docs/benchmark/throughput-1thread-2048bytes.png)
![](docs/benchmark/throughput-1thread-4096bytes.png)
![](docs/benchmark/throughput-1thread-8192bytes.png)



# Examples

## An echo TCP server

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

### An echo HTTP server

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


### An echo UDP server

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

# 用户列表

<img src="http://i.imgur.com/dcHpCm4.png" height = "100" width = "120" alt="Qihoo">

> 欢迎申请加入该名单 ;-)

# Thanks

1. 感谢[奇虎360公司](http://www.360.cn "http://www.360.cn")对该项目的支持
1. 感谢[libevent], [glog](https://github.com/google/glog), [gtest](https://github.com/google/googletest), [Golang]等项目
1. [evpp]深度参考了[muduo]项目的实现和设计，非常感谢[Chen Shuo](https://github.com/chenshuo "https://github.com/chenshuo")


[evpp]:https://github.com/Qihoo360/evpp
[Golang]:https://golang.org
[muduo]:https://github.com/chenshuo/muduo
[libevent]:https://github.com/libevent/libevent