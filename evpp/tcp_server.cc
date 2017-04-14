#include "evpp/inner_pre.h"

#include "evpp/tcp_server.h"
#include "evpp/listener.h"
#include "evpp/tcp_conn.h"
#include "evpp/libevent_headers.h"

namespace evpp {
TCPServer::TCPServer(EventLoop* loop,
                     const std::string& laddr,
                     const std::string& name,
                     uint32_t thread_num)
    : loop_(loop)
    , listen_addr_(laddr)
    , name_(name)
    , conn_fn_(&internal::DefaultConnectionCallback)
    , msg_fn_(&internal::DefaultMessageCallback)
    , next_conn_id_(0) {
    tpool_.reset(new EventLoopThreadPool(loop_, thread_num));
}

TCPServer::~TCPServer() {
    LOG_TRACE << "TCPServer::~TCPServer()";
    assert(connections_.empty());
    assert(!listener_);
    if (tpool_) {
        assert(tpool_->IsStopped());
        tpool_.reset();
    }
}

bool TCPServer::Init() {
    assert(status_ == kNull);
    listener_.reset(new Listener(loop_, listen_addr_));
    listener_->Listen();
    status_.store(kInitialized);
    return true;
}

bool TCPServer::Start() {
    assert(status_ == kInitialized);
    status_.store(kStarting);
    assert(listener_.get());
    bool rc = tpool_->Start(true);
    if (rc) {
        assert(tpool_->IsRunning());
        listener_->SetNewConnectionCallback(
            std::bind(&TCPServer::HandleNewConn,
                      this,
                      std::placeholders::_1,
                      std::placeholders::_2,
                      std::placeholders::_3));

        // We must set status_ to kRunning firstly and then we can accept new
        // connections. If we use the following code :
        //     listener_->Accept();
        //     status_.store(kRunning);
        // there is chance : we have accept a connection but status_ is not
        // kRunning that will make the assert(status_ == kRuning) failed in
        // TCPServer::HandleNewConn.
        status_.store(kRunning);
        listener_->Accept();
    }
    return rc;
}

bool TCPServer::IsRunning() const {
    if (status_ != kRunning) {
        return false;
    }

    if (!loop_->IsRunning()) {
        return false;
    }

    if (!tpool_->IsRunning()) {
        return false;
    }

    assert(loop_->IsRunning() && tpool_->IsRunning());
    return true;
}

bool TCPServer::IsStopped() const {
    if (!loop_->IsStopped()) {
        return false;
    }

    if (!tpool_->IsStopped()) {
        return false;
    }

    assert(loop_->IsStopped() && tpool_->IsStopped());
    return true;
}

void TCPServer::Stop(Functor on_stopped_cb) {
    LOG_TRACE << "this=" << this << "Entering TCPServer::Stop";
    assert(status_ == kRunning);
    status_.store(kStopping);
    loop_->RunInLoop(std::bind(&TCPServer::StopInLoop, this, on_stopped_cb));
}

void TCPServer::StopThreadPool() {
    LOG_INFO << "this=" << this << " StopThreadPool pool=" << tpool_.get();
    assert(loop_->IsInLoopThread());
    tpool_->Stop(true);
    assert(tpool_->IsStopped());

    // Make sure all the working threads totally stopped.
    tpool_->Join();
    tpool_.reset();
}

void TCPServer::StopInLoop(Functor on_stopped_cb) {
    LOG_TRACE << "this=" << this << "Entering TCPServer::StopInLoop";
    assert(loop_->IsInLoopThread());
    listener_->Stop();
    listener_.reset();

    if (connections_.empty()) {
        // Stop all the working threads now.
        LOG_INFO << "this=" << this << " TCPServer::StopInLoop no connections";
        StopThreadPool();
        status_.store(kStopped);
        if (on_stopped_cb) {
            on_stopped_cb();
            on_stopped_cb = Functor();
        }
    } else {
        for (auto& c : connections_) {
            c.second->Close();
        }

        stopped_cb_ = on_stopped_cb;

        // The working threads will be stopped after all the connections closed.
    }

    LOG_TRACE << "TCPServer::StopInLoop exited, status=" << ToString();
}

void TCPServer::HandleNewConn(int sockfd,
                              const std::string& remote_addr/*ip:port*/,
                              const struct sockaddr_in* raddr) {
    assert(loop_->IsInLoopThread());
    if (status_.load() == kStopping) {
        LOG_WARN << "The server is at stopping status. Discard this socket fd=" << sockfd << " remote_addr=" << remote_addr;
        EVUTIL_CLOSESOCKET(sockfd);
        return;
    }

    assert(IsRunning());
    EventLoop* io_loop = GetNextLoop(raddr);
    std::string n = name_ + "-" + remote_addr + "#" + std::to_string(next_conn_id_++); // TODO use string buffer
    TCPConnPtr conn(new TCPConn(io_loop, n, sockfd, listen_addr_, remote_addr));
    assert(conn->type() == TCPConn::kIncoming);
    conn->SetMessageCallback(msg_fn_);
    conn->SetConnectionCallback(conn_fn_);
    conn->SetCloseCallback(std::bind(&TCPServer::RemoveConnection, this, std::placeholders::_1));
    io_loop->RunInLoop(std::bind(&TCPConn::OnAttachedToLoop, conn));
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
    LOG_INFO << "this=" << this << " TCPServer::RemoveConnection conn=" << conn.get() << " fd="<< conn->fd();
    auto f = [this, conn]() {
        // Remove the connection in the listening EventLoop
        LOG_INFO << "this=" << this << " TCPServer::RemoveConnection conn=" << conn.get() << " fd="<< conn->fd();
        assert(this->loop_->IsInLoopThread());
        this->connections_.erase(conn->name());
        if (status_ == kStopping && this->connections_.empty()) {
            // At last, we stop all the working threads
            LOG_INFO << "this=" << this << " TCPServer::RemoveConnection stop thread pool";
            StopThreadPool();
            status_.store(kStopped);
            if (stopped_cb_) {
                stopped_cb_();
                stopped_cb_ = Functor();
            }
        }
    };
    loop_->RunInLoop(f);
}

}
