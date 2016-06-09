#include "evpp/inner_pre.h"

#include "evpp/tcp_server.h"
#include "evpp/listener.h"

namespace evpp {
    TCPServer::TCPServer(EventLoop* loop, 
                         const std::string& listen_addr, 
                         const std::string& name, 
                         int thread_num) 
                         : loop_(loop)
                         , listen_addr_(listen_addr)
                         , name_(name) {
        tpool_.reset(new EventLoopThreadPool(loop));
        tpool_->SetThreadNum(thread_num);
    }

    bool TCPServer::Start() {
        acceptor_.reset(new Listener(loop_, listen_addr_));
        acceptor_->Start();
        return true;
    }
}


