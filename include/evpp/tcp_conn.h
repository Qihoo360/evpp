#pragma once

#include "evpp/inner_pre.h"
#include "evpp/buffer.h"

namespace evpp {
    
    class EventLoop;
    class FdChannel;

    class EVPP_EXPORT TCPConn {
    public:
        TCPConn(EventLoop* loop,
                const std::string& name,
                int sockfd,
                const std::string& local_addr,
                const std::string& peer_addr);

    private:
        EventLoop* loop_;
        std::string name_;
        std::string local_addr_;
        std::string peer_addr_;
        xstd::shared_ptr<FdChannel> chan_;
        
    };
}