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
                         , next_conn_id_(0) {
        threads_dispatch_policy_ = kRoundRobin;
        tpool_.reset(new EventLoopThreadPool(loop_, thread_num));
    }

    TCPServer::~TCPServer() {
        LOG_TRACE << "TCPServer::~TCPServer()";
        assert(tpool_->IsStopped());
        assert(!listener_->listening());
        assert(connections_.empty());
        listener_.reset();
        tpool_.reset();
    }

    bool TCPServer::Start() {
        tpool_->Start(true);
        listener_.reset(new Listener(loop_, listen_addr_));
        listener_->Listen();
        listener_->SetNewConnectionCallback(
            std::bind(&TCPServer::HandleNewConn,
            this,
            std::placeholders::_1,
            std::placeholders::_2));
        return true;
    }

    void TCPServer::Stop() {
        loop_->RunInLoop(std::bind(&TCPServer::StopInLoop, this));
    }

    void TCPServer::StopInLoop() {
        LOG_TRACE << "Entering TCPServer::StopInLoop";
        listener_->Stop();
        auto it = connections_.begin();
        auto ite = connections_.end();
        for (; it != ite; ++it) {
            it->second->Close();
        }
        tpool_->Stop(true);
        LOG_TRACE << "TCPServer::StopInLoop exited";
    }

    void TCPServer::HandleNewConn(int sockfd, const std::string& remote_addr/*ip:port*/) {
        assert(loop_->IsInLoopThread());
        EventLoop* io_loop = GetNextLoop(remote_addr);
        char buf[64];
        snprintf(buf, sizeof buf, "-%s#%" PRIu64, remote_addr.c_str(), next_conn_id_++);
        std::string n = name_ + buf;

        TCPConnPtr conn(new TCPConn(io_loop, n, sockfd, listen_addr_, remote_addr));
        assert(conn->type() == TCPConn::kIncoming);
        conn->SetMessageCallback(msg_fn_);
        conn->SetConnectionCallback(conn_fn_);
        conn->SetCloseCallback(std::bind(&TCPServer::RemoveConnection, this, std::placeholders::_1));
        io_loop->RunInLoop(std::bind(&TCPConn::OnAttachedToLoop, conn.get()));
        connections_[n] = conn;
    }

    EventLoop* TCPServer::GetNextLoop(const std::string& raddr) {
        if (threads_dispatch_policy_ == kRoundRobin) {
            return tpool_->GetNextLoop();
        } else {
            assert(threads_dispatch_policy_ == kIPAddressHashing);
            //TODO efficient improve. Using the sockaddr_in to calculate the hash value of the remote address instead of std::string
            auto index = raddr.rfind(':');
            assert(index != std::string::npos);
            auto hash = std::hash<std::string>()(std::string(raddr.data(), index));
            return tpool_->GetNextLoopWithHash(hash);
        }
    }

    void TCPServer::RemoveConnection(const TCPConnPtr& conn) {
        loop_->RunInLoop(std::bind(&TCPServer::RemoveConnectionInLoop, this, conn));
    }

    void TCPServer::RemoveConnectionInLoop(const TCPConnPtr& conn) {
        assert(loop_->IsInLoopThread());
        connections_.erase(conn->name());
    }


}


