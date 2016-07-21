#include "evpp/inner_pre.h"

#include "evpp/tcp_server.h"
#include "evpp/listener.h"
#include "evpp/tcp_conn.h"

namespace evpp {
TCPServer::TCPServer(EventLoop* loop,
                     const std::string& listen_addr,
                     const std::string& name,
                     uint32_t thread_num)
    : loop_(loop)
    , listen_addr_(listen_addr)
    , name_(name)
    , next_conn_id_(0) {
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
                  std::placeholders::_2,
                  std::placeholders::_3));
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

void TCPServer::HandleNewConn(int sockfd,
                              const std::string& remote_addr/*ip:port*/,
                              const struct sockaddr_in* raddr) {
    assert(loop_->IsInLoopThread());
    EventLoop* io_loop = GetNextLoop(raddr);
    char buf[64] = {};
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

EventLoop* TCPServer::GetNextLoop(const struct sockaddr_in* raddr) {
    if (IsRoundRobin()) {
        return tpool_->GetNextLoop();
    } else {
        return tpool_->GetNextLoopWithHash(raddr->sin_addr.s_addr);
    }
}

void TCPServer::RemoveConnection(const TCPConnPtr& conn) {
    auto f = [this](const TCPConnPtr& c) {
        // Remove the connection in the listener EventLoop
        assert(this->loop_->IsInLoopThread());
        this->connections_.erase(c->name());
    };
    loop_->RunInLoop(std::bind(f, conn));
}

}
