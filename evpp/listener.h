#pragma once

#include "evpp/inner_pre.h"
#include "evpp/timestamp.h"

namespace evpp {
class EventLoop;
class FdChannel;

class EVPP_EXPORT Listener {
public:
    typedef std::function <
    void(evpp_socket_t sockfd,
         const std::string& /*remote address with format "ip:port"*/,
         const struct sockaddr_in* /*remote address*/) >
    NewConnectionCallback;
    Listener(EventLoop* loop, const std::string& addr/*local listening address : ip:port*/);
    ~Listener();

    // socket listen
    void Listen(int backlog = SOMAXCONN);

    // nonblocking accept
    void Accept();

    void Stop();

    void SetNewConnectionCallback(NewConnectionCallback cb) {
        new_conn_fn_ = cb;
    }

private:
    void HandleAccept();

private:
    evpp_socket_t fd_ = -1;// The listening socket fd
    EventLoop* loop_;
    std::string addr_;
    std::unique_ptr<FdChannel> chan_;
    NewConnectionCallback new_conn_fn_;
};
}


