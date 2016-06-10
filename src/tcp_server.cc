#include "evpp/inner_pre.h"

#include "evpp/tcp_server.h"
#include "evpp/listener.h"
#include "evpp/tcp_conn.h"

namespace evpp {
    TCPServer::TCPServer(EventLoop* loop,
                         const std::string& listen_addr,
                         const std::string& name,
                         int thread_num)
                         : loop_(loop)
                         , listen_addr_(listen_addr)
                         , name_(name)
                         , nextConnId_(0) {
        threads_dispatch_policy_ = kRoundRobin;
        tpool_.reset(new EventLoopThreadPool(loop));
        tpool_->SetThreadNum(thread_num);
    }

    bool TCPServer::Start() {
        listener_.reset(new Listener(loop_, listen_addr_));
        listener_->Start();
        listener_->SetNewConnectionCallback(
            xstd::bind(&TCPServer::HandleNewConn,
            this,
            xstd::placeholders::_1,
            xstd::placeholders::_2));
        return true;
    }

    void TCPServer::HandleNewConn(int sockfd, const std::string& remote_addr/*ip:port*/) {
        EventLoop* io_loop = GetNextLoop(remote_addr);
        char buf[64];
        snprintf(buf, sizeof buf, "-%s#%lu", remote_addr.c_str(), nextConnId_++);
        std::string n = name_ + buf;

        TcpConnectionPtr conn(new TCPConn(io_loop, n, sockfd, listen_addr_, remote_addr));
        conn->SetMesageHandler(messageCallback_);
        conn->SetCloseCallback(xstd::bind(&TCPServer::RemoveConnection, this, xstd::placeholders::_1));
        io_loop->RunInLoop(xstd::bind(&TCPConn::OnAttachedToLoop, conn.get()));
        connections_[n] = conn;
    }

    EventLoop* TCPServer::GetNextLoop(const std::string& remote_addr) {
        if (threads_dispatch_policy_ == kRoundRobin) {
            return tpool_->GetNextLoop();
        } else {
            assert(threads_dispatch_policy_ == kIPAddressHashing);
            //TODO efficient improve. Using the sockaddr_in to calculate the hash value of the remote hash
            auto index = remote_addr.rfind(':');
            assert(index != std::string::npos);
            auto hash = std::hash<std::string>()(std::string(remote_addr.data(), index));
            return tpool_->GetNextLoopWithHash(hash);
        }
    }

    void TCPServer::RemoveConnection(const TcpConnectionPtr& conn) {
        loop_->RunInLoop(xstd::bind(&TCPServer::RemoveConnectionInLoop, this, conn));
    }

    void TCPServer::RemoveConnectionInLoop(const TcpConnectionPtr& conn) {
        connections_.erase(conn->name());
    }
}


