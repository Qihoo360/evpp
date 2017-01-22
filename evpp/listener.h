#pragma once

#include "evpp/inner_pre.h"
#include "evpp/timestamp.h"

namespace evpp {
class EventLoop;
class FdChannel;

class EVPP_EXPORT Listener {
public:
    typedef std::function <
    void(int sockfd,
         const std::string& /*remote address with format "ip:port"*/,
         const struct sockaddr_in* /*remote addr*/) > NewConnectionCallback;
    Listener(EventLoop* loop, const std::string& addr/*local listening ip:port*/);
    ~Listener();

    void Listen();
    void Stop();

    void SetNewConnectionCallback(NewConnectionCallback cb) {
        new_conn_fn_ = cb;
    }

    bool listening() const {
        return listening_;
    }
private:
    void HandleAccept(Timestamp ts);

private:
    int fd_;// The listener socket fd
    EventLoop* loop_;
    bool listening_;
    std::string addr_;
    std::unique_ptr<FdChannel> chan_;
    NewConnectionCallback new_conn_fn_;
};
}


