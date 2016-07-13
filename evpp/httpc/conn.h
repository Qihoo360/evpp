#pragma once

#include "evpp/inner_pre.h"
#include "evpp/event_loop.h"

struct evhttp_connection;
namespace evpp {
namespace httpc {
class ConnPool;
class EVPP_EXPORT Conn {
public:
    Conn(EventLoop* loop, const std::string& host, int port, Duration timeout);
    ~Conn();

    bool Init();
    void Close();

    EventLoop* loop() {
        return loop_;
    }
    struct evhttp_connection* evhttp_conn() {
        return evhttp_conn_;
    }
    const std::string& host() const {
        return host_;
    }
    int port() const {
        return port_;
    }
    Duration timeout() const {
        return timeout_;
    }
private:
    friend class ConnPool;
    Conn(ConnPool* pool, EventLoop* loop);
    ConnPool* pool() {
        return pool_;
    }
private:
    EventLoop* loop_;
    ConnPool* pool_;
    std::string host_;
    int port_;
    Duration timeout_;
    struct evhttp_connection* evhttp_conn_;
};
} // httpc
} // evpp