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

    void TCPClient::Connect() {
        loop_->AssertInLoopThread();
        connector_.reset(new Connector(loop_, remote_addr_, connection_timeout_));
        connector_->SetNewConnectionCallback(xstd::bind(&TCPClient::OnConnected, this, xstd::placeholders::_1, xstd::placeholders::_2));
        connector_->Start();
    }

    void TCPClient::Disconnect() {
        auto_reconnect_.store(0);
        if (conn_) {
            conn_->Close();
        }
    }

    void TCPClient::Reconnect() {
        return Connect();
    }

    void TCPClient::OnConnected(int sockfd, const std::string& laddr) {
        loop_->AssertInLoopThread();
        conn_ = TCPConnPtr(new TCPConn(loop_, name_, sockfd, laddr, remote_addr_));
        conn_->set_type(TCPConn::kOutgoing);
        conn_->SetMesageHandler(msg_fn_);
        conn_->SetConnectionHandler(conn_fn_);
        conn_->SetCloseCallback(xstd::bind(&TCPClient::OnRemoveConnection, this, xstd::placeholders::_1));
        conn_->OnAttachedToLoop();
    }

    void TCPClient::OnRemoveConnection(const TCPConnPtr& conn) {
        loop_->AssertInLoopThread();
        if (auto_reconnect_.load() != 0) {
            Reconnect();
        }
    }

    
}
