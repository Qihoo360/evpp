#include "evpp/inner_pre.h"

#include "evpp/tcp_client.h"
#include "evpp/libevent_headers.h"
#include "evpp/tcp_conn.h"
#include "evpp/fd_channel.h"
#include "evpp/connector.h"

namespace evpp {
    TCPClient::TCPClient(EventLoop* l, const std::string& raddr, const std::string& n)
        : loop_(l), remote_addr_(raddr), name_(n), auto_reconnect_(1), connection_timeout_(3.0) {
    }

    TCPClient::~TCPClient() {
        auto_reconnect_.store(0);
        TCPConnPtr c = conn();
        assert(c->IsDisconnected());
        conn_.reset();
    }

    void TCPClient::Connect() {
        loop_->RunInLoop(std::bind(&TCPClient::ConnectInLoop, this));
    }

    void TCPClient::ConnectInLoop() {
        loop_->AssertInLoopThread();
        connector_.reset(new Connector(loop_, remote_addr_, connection_timeout_));
        connector_->SetNewConnectionCallback(std::bind(&TCPClient::OnConnection, this, std::placeholders::_1, std::placeholders::_2));
        connector_->Start();
    }

    void TCPClient::Disconnect() {
        loop_->RunInLoop(std::bind(&TCPClient::DisconnectInLoop, this));
    }

    void TCPClient::DisconnectInLoop() {
        loop_->AssertInLoopThread();
        auto_reconnect_.store(0);
        if (conn_) {
            conn_->Close();
        } else {
            // When connector_ is connecting to the remote server ...
            assert(connector_ && !connector_->IsConnected());
        }
        connector_.reset(); // Free connector_ in loop thread immediately
    }

    void TCPClient::Reconnect() {
        return Connect();
    }

    void TCPClient::OnConnection(int sockfd, const std::string& laddr) {
        if (sockfd < 0) {
            LOG_INFO << "Failed to connect to " << remote_addr_ << ". errno=" << errno << " " << strerror(errno);
            conn_fn_(TCPConnPtr());
            if (auto_reconnect_.load() != 0) {
                Reconnect();
            }
            return;
        }

        LOG_INFO << "Successfully connected to " << remote_addr_;
        loop_->AssertInLoopThread();
        TCPConnPtr c = TCPConnPtr(new TCPConn(loop_, name_, sockfd, laddr, remote_addr_));
        c->set_type(TCPConn::kOutgoing);
        c->SetMessageCallback(msg_fn_);
        c->SetConnectionCallback(conn_fn_);
        c->SetCloseCallback(std::bind(&TCPClient::OnRemoveConnection, this, std::placeholders::_1));

        {
            std::lock_guard<std::mutex> guard(mutex_);
            conn_ = c;
        }

        c->OnAttachedToLoop();
    }

    void TCPClient::OnRemoveConnection(const TCPConnPtr& c) {
        assert(c.get() == conn_.get());
        loop_->AssertInLoopThread();
        if (auto_reconnect_.load() != 0) {
            Reconnect();
        }
    }

    TCPConnPtr TCPClient::conn() const {
        if (loop_->IsInLoopThread()) {
            return conn_;
        } else {
            // If it is not in the loop thread, we should add a lock here
            std::lock_guard<std::mutex> guard(mutex_);
            TCPConnPtr c = conn_;
            return c;
        }
    }
}
