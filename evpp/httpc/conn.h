#pragma once

#include "evpp/inner_pre.h"
#include "evpp/event_loop.h"

#if defined(EVPP_HTTP_CLIENT_SUPPORTS_SSL)
#include <event2/bufferevent_ssl.h>
#include <openssl/ssl.h>
#endif

struct evhttp_connection;
namespace evpp {
namespace httpc {
class ConnPool;
class EVPP_EXPORT Conn {
public:
    Conn(EventLoop* loop, const std::string& host, int port,
#if defined(EVPP_HTTP_CLIENT_SUPPORTS_SSL)
        bool enable_ssl,
#endif
        Duration timeout);
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
#if defined(EVPP_HTTP_CLIENT_SUPPORTS_SSL)
    bool enable_ssl() const {
        return enable_ssl_;
    }
    struct bufferevent* bufferevent() const {
        return bufferevent_;
    }
#endif
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
#if defined(EVPP_HTTP_CLIENT_SUPPORTS_SSL)
    bool enable_ssl_;
    SSL* ssl_;
    struct bufferevent* bufferevent_;
#endif
    Duration timeout_;
    struct evhttp_connection* evhttp_conn_;
};
} // httpc
} // evpp
