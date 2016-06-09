#include "evpp/inner_pre.h"

#include "evpp/tcp_conn.h"
#include "evpp/fd_channel.h"
#include "evpp/event_loop.h"

namespace evpp {
    TCPConn::TCPConn(EventLoop* loop,
                     const std::string& name,
                     int sockfd,
                     const std::string& local_addr,
                     const std::string& peer_addr) 
                     : loop_(loop)
                     , name_(name)
                     , local_addr_(local_addr)
                     , peer_addr_(peer_addr) {
        chan_.reset(new FdChannel(loop->event_base(), sockfd, true, true));
        chan_->Start();
    }
}