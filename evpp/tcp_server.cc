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
    , conn_fn_(&internal::DefaultConnectionCallback)
    , msg_fn_(&internal::DefaultMessageCallback)
    , next_conn_id_(0) {
    tpool_.reset(new EventLoopThreadPool(loop_, thread_num));
}

TCPServer::~TCPServer() {
    LOG_TRACE << "TCPServer::~TCPServer()";
    assert(tpool_->IsStopped());
    assert(connections_.empty());
    assert(!listener_);
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
    listener_.reset();

    for (auto &c : connections_) {
        c.second->Close();
    }

    tpool_->Stop(true);
    assert(tpool_->IsStopped());
    LOG_TRACE << "TCPServer::StopInLoop exited";
}

void TCPServer::HandleNewConn(int sockfd,
                              const std::string& remote_addr/*ip:port*/,
                              const struct sockaddr_in* raddr) {
    assert(loop_->IsInLoopThread());
    EventLoop* io_loop = GetNextLoop(raddr);
    std::string n = name_ + "-" + remote_addr + "#" + std::to_string(next_conn_id_++); // TODO use string buffer
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
    auto f = [=]() {
        // Remove the connection in the listener EventLoop
        assert(this->loop_->IsInLoopThread());
        this->connections_.erase(conn->name());
    };
    loop_->RunInLoop(f);
}

}
