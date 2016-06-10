#pragma once

#include "evpp/inner_pre.h"
#include "evpp/base/timestamp.h"

namespace evpp {
    class EventLoop;
    class FdChannel;

    class EVPP_EXPORT Listener {
    public:
        typedef xstd::function<void(int sockfd, const std::string& /*ip:port*/)> NewConnectionCallback;
        Listener(EventLoop* loop, const std::string& addr/*ip:port*/);
        ~Listener();

        void Listen();
        void Stop();

        void SetNewConnectionCallback(NewConnectionCallback cb) {
            new_conn_fn_ = cb;
        }
    private:
        void HandleAccept(base::Timestamp ts);

    private:
        int fd_;// The listener socket fd
        xstd::shared_ptr<FdChannel> chan_;
        EventLoop* loop_;
        std::string addr_;
        NewConnectionCallback new_conn_fn_;
    };
}


