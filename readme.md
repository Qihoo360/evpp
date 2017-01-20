evpp
---

`evpp` is a modern C++ network library for develop high performance network servers in TCP/UDP/HTTP protocols.
`evpp` provides a TCP Server to support multi-threaded nonblocking server. And it also provides a HTTP and UDP Server to support http and udp prococol.

`evpp` provides:

1. A nonblocking TCP server
1. A nonblocking TCP client
1. A nonblocking HTTP server
1. A nonblocking HTTP client
1. A blocking UDP server
1. EventLoop
1. Thread pool
1. Timer

# [Quick Start](quick_start.md)

# Usage

## A echo TCP server

```cpp
#include <evpp/exp.h>
#include <evpp/tcp_server.h>
#include <evpp/buffer.h>
#include <evpp/tcp_conn.h>

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


void OnConnection(const evpp::TCPConnPtr& conn) {
    if (conn->IsConnected()) {
        LOG_INFO << "Accept a new connection from " << conn->remote_addr();
    } else {
        LOG_INFO << "Disconnected from " << conn->remote_addr();
    }
}


int main(int argc, char* argv[]) {
    std::string port = "9099";
    if (argc == 2) {
        port = argv[1];
    }
    std::string addr = std::string("0.0.0.0:") + port;
    evpp::EventLoop loop;
    evpp::TCPServer server(&loop, addr, "TCPEcho", 0);
    server.SetMessageCallback(&OnMessage);
    server.SetConnectionCallback(&OnConnection);
    server.Start();
    loop.Run();
    return 0;
}
```

### A echo HTTP server

```cpp
#include <evpp/exp.h>
#include <evpp/http/http_server.h>

void DefaultHandler(evpp::EventLoop* loop,
                    const evpp::http::ContextPtr& ctx,
                    const evpp::http::HTTPSendResponseCallback& cb) {
    std::stringstream oss;
    oss << "func=" << __FUNCTION__ << " OK"
        << " ip=" << ctx->remote_ip << "\n"
        << " uri=" << ctx->uri << "\n"
        << " body=" << ctx->body.ToString() << "\n";
    ctx->AddResponseHeader("Content-Type", "application/octet-stream");
    ctx->AddResponseHeader("Server", "evpp");
    cb(oss.str());
}

void RequestHandler(evpp::EventLoop* loop,
                    const evpp::http::ContextPtr& ctx,
                    const evpp::http::HTTPSendResponseCallback& cb) {
    cb(ctx->body.ToString());
}

int main(int argc, char* argv[]) {
    std::vector<int> ports = {9009, 23456, 23457};
    int port = 29099;
    int thread_num = 2;

    if (argc > 1) {
        if (std::string("-h") == argv[1] ||
            std::string("--h") == argv[1] ||
            std::string("-help") == argv[1] ||
            std::string("--help") == argv[1]) {
            std::cout << "usage : " << argv[0] << " <listen_port> <thread_num>\n";
            std::cout << " e.g. : " << argv[0] << " 8080 24\n";
            return 0;
        }
    }

    if (argc == 2) {
        port = atoi(argv[1]);
    }
    ports.push_back(port);

    if (argc == 3) {
        port = atoi(argv[1]);
        thread_num = atoi(argv[2]);
    }
    evpp::http::Server server(thread_num);
    server.SetThreadDispatchPolicy(evpp::ThreadDispatchPolicy::kIPAddressHashing);
    server.RegisterDefaultHandler(&DefaultHandler);
    server.RegisterHandler("/echo", &RequestHandler);
    server.Start(ports);
    while (!server.IsStopped()) {
        usleep(1);
    }
    return 0;
}

```

# Thanks

Thanks for the support of [Qihoo360](http://www.360.cn "http://www.360.cn").

Thanks for `libevent`, `glog`, `gtest` projects. There are great open source projects.

`evpp` is highly inspired by [muduo](https://github.com/chenshuo/muduo "https://github.com/chenshuo/muduo"). Thanks for the great work of [Chen Shuo](https://github.com/chenshuo "https://github.com/chenshuo")

