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
        conn_.reset();
    }

    void TCPClient::Connect() {
        loop_->RunInLoop(xstd::bind(&TCPClient::ConnectInLoop, this));
    }

    void TCPClient::ConnectInLoop() {
        loop_->AssertInLoopThread();
        connector_.reset(new Connector(loop_, remote_addr_, connection_timeout_));
        connector_->SetNewConnectionCallback(xstd::bind(&TCPClient::OnConnection, this, xstd::placeholders::_1, xstd::placeholders::_2));
        connector_->Start();
    }

    void TCPClient::Disconnect() {
        loop_->RunInLoop(xstd::bind(&TCPClient::DisconnectInLoop, this));
    }

    void TCPClient::DisconnectInLoop() {
        loop_->AssertInLoopThread();
        auto_reconnect_.store(0);
        if (conn_) {
            conn_->Close();
        } else {
            // When connector_ is connecting to the remote server ...
            assert(connector_ && !connector_->IsConnected());
            connector_.reset();
        }
    }

    void TCPClient::Reconnect() {
        return Connect();
    }

    void TCPClient::OnConnection(int sockfd, const std::string& laddr) {
        if (sockfd < 0) {
            LOG_INFO << "Failed to connect to " << remote_addr_ << ". errno=" << errno << " " << strerror(errno);
            if (auto_reconnect_.load() != 0) {
                Reconnect();
            }
            return;
        }

        LOG_INFO << "Successfully connected to " << remote_addr_;
        loop_->AssertInLoopThread();
        conn_ = TCPConnPtr(new TCPConn(loop_, name_, sockfd, laddr, remote_addr_));
        conn_->set_type(TCPConn::kOutgoing);
        conn_->SetMesageHandler(msg_fn_);
        conn_->SetConnectionHandler(conn_fn_);
        conn_->SetCloseCallback(xstd::bind(&TCPClient::OnRemoveConnection, this, xstd::placeholders::_1));
        conn_->OnAttachedToLoop();
    }

    void TCPClient::OnRemoveConnection(const TCPConnPtr& conn) {
        assert(conn.get() == conn_.get());
        loop_->AssertInLoopThread();
        if (auto_reconnect_.load() != 0) {
            Reconnect();
        }
    }


    
}
