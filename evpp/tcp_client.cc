#include "evpp/inner_pre.h"

#include "evpp/tcp_client.h"
#include "evpp/libevent_headers.h"
#include "evpp/tcp_conn.h"
#include "evpp/fd_channel.h"
#include "evpp/connector.h"

namespace evpp {
TCPClient::TCPClient(EventLoop* l, const std::string& raddr, const std::string& n)
    : loop_(l)
    , remote_addr_(raddr)
    , name_(n)
    , auto_reconnect_(true)
    , reconnect_interval_(3.0)
    , connecting_timeout_(3.0)
    , conn_fn_(&internal::DefaultConnectionCallback)
    , msg_fn_(&internal::DefaultMessageCallback) {
}

TCPClient::~TCPClient() {
    assert(!connector_.get());
    auto_reconnect_.store(false);
    TCPConnPtr c = conn();
    assert(c->IsDisconnected());
    conn_.reset();
}

void TCPClient::Connect() {
    auto f = [this]() {
        loop_->AssertInLoopThread();
        connector_.reset(new Connector(loop_, this));
        connector_->SetNewConnectionCallback(std::bind(&TCPClient::OnConnection, this, std::placeholders::_1, std::placeholders::_2));
        connector_->Start();
    };
    loop_->RunInLoop(f);
}

void TCPClient::Disconnect() {
    loop_->RunInLoop(std::bind(&TCPClient::DisconnectInLoop, this));
}

void TCPClient::DisconnectInLoop() {
    loop_->AssertInLoopThread();
    auto_reconnect_.store(false);

    if (conn_) {
        conn_->Close();
    } else {
        // When connector_ is connecting to the remote server ...
        assert(connector_ && !connector_->IsConnected());
    }

    if (connector_->IsConnected() || connector_->IsDisconnected()) {
        LOG_TRACE << "Do nothing, Connector::status=" << connector_->status();
    } else {
        // When connector_ is trying to connect to the remote server we should cancel it to release the resources.
        connector_->Cancel();
    }

    connector_.reset(); // Free connector_ in loop thread immediately
}

void TCPClient::Reconnect() {
    LOG_INFO << "Try to reconnect to " << remote_addr_ << " in " << reconnect_interval_.Seconds() << "s again";
    Connect();
}

void TCPClient::OnConnection(int sockfd, const std::string& laddr) {
    if (sockfd < 0) {
        LOG_INFO << "Failed to connect to " << remote_addr_ << ". errno=" << errno << " " << strerror(errno);
        // ��ĳЩ�����£���Ҫ������ʧ�ܷ������ϲ������
        // ע�⣺���޷����ӵ�������ʱ�����ڿͻ��˲��ϵ����ԣ��ᵼ���ϲ������Ҳ�᲻�ϵ��յ��ûص�֪ͨ
        conn_fn_(TCPConnPtr(new TCPConn(loop_, "", sockfd, "", "")));
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

    if (auto_reconnect_.load()) {
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
