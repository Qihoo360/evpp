#include "evpp/inner_pre.h"

#include "evpp/connector.h"
#include "evpp/event_loop.h"
#include "evpp/fd_channel.h"
#include "evpp/sockets.h"
#include "evpp/libevent_headers.h"
#include "evpp/dns_resolver.h"
#include "evpp/tcp_client.h"

namespace evpp {
Connector::Connector(EventLoop* l, TCPClient* client)
    : status_(kDisconnected)
    , loop_(l)
    , owner_tcp_client_(client)
    , remote_addr_(client->remote_addr())
    , timeout_(client->connecting_timeout())
    , fd_(-1)
    , own_fd_(false) {
    LOG_INFO << "Connector::Connector this=" << this << " raddr=" << remote_addr_;
    raddr_ = sock::ParseFromIPPort(remote_addr_.data());
}

Connector::~Connector() {
    assert(loop_->IsInLoopThread());

    if (!IsConnected()) {
        // A connected tcp-connection's sockfd has been transfered to TCPConn.
        // But the sockfd of unconnected tcp-connections need to be closed by myself.
        LOG_TRACE << "Connector::~Connector close(" << chan_->fd() << ")";
        assert(own_fd_);
        assert(chan_->fd() == fd_);
        EVUTIL_CLOSESOCKET(fd_);
        fd_ = INVALID_SOCKET;
    }

    assert(fd_ < 0);
    chan_.reset();
}

void Connector::Start() {
    LOG_INFO << "Try to connect " << remote_addr_ << " status=" << StatusToString();
    assert(loop_->IsInLoopThread());

    timer_.reset(new TimerEventWatcher(loop_, std::bind(&Connector::OnConnectTimeout, shared_from_this()), timeout_));
    timer_->Init();
    timer_->AsyncWait();

    if (raddr_.sin_addr.s_addr == 0) {
        LOG_INFO << "The remote address " << remote_addr_ << " is a host, try to resolve its IP address.";
        status_ = kDNSResolving;
        auto index = remote_addr_.rfind(':');
        assert(index != std::string::npos);
        auto host = std::string(remote_addr_.data(), index);
        dns_resolver_.reset(new DNSResolver(loop_, host, timeout_, std::bind(&Connector::OnDNSResolved, shared_from_this(), std::placeholders::_1)));
        dns_resolver_->Start();
        return;
    }

    Connect();
}


void Connector::Cancel() {
    LOG_INFO << "Cancel to connect " << remote_addr_ << " status=" << StatusToString();
    assert(loop_->IsInLoopThread());
    if (dns_resolver_) {
        dns_resolver_->Cancel();
    }

    assert(timer_);
    timer_->Cancel();
    chan_->DisableAllEvent();
    chan_->Close();
}

void Connector::Connect() {
    fd_ = sock::CreateNonblockingSocket();
    own_fd_ = true;
    assert(fd_ >= 0);
    int rc = ::connect(fd_, sock::sockaddr_cast(&raddr_), sizeof(raddr_));
    if (rc != 0) {
        int serrno = errno;
        if (!EVUTIL_ERR_CONNECT_RETRIABLE(serrno)) {
            HandleError();
            return;
        }
    }

    status_ = kConnecting;

    chan_.reset(new FdChannel(loop_, fd_, false, true));
    LOG_TRACE << "this=" << this << " new FdChannel p=" << chan_.get() << " fd=" << chan_->fd();
    chan_->SetWriteCallback(std::bind(&Connector::HandleWrite, shared_from_this()));
    chan_->AttachToLoop();
}

void Connector::HandleWrite() {
    if (status_ == kDisconnected) {
        // 这里有可能是超时了，但回调时间已经派发到队列中，后来才调用。
        LOG_INFO << "fd=" << chan_->fd() << " remote_addr=" << remote_addr_ << " receive write event when socket is closed";
        return;
    }

    assert(status_ == kConnecting);
    int err = 0;
    socklen_t len = sizeof(len);
    if (getsockopt(chan_->fd(), SOL_SOCKET, SO_ERROR, (char*)&err, (socklen_t*)&len) != 0) {
        err = errno;
        LOG_ERROR << "getsockopt failed err=" << err << " " << strerror(err);
    }

    if (err != 0) {
        EVUTIL_SET_SOCKET_ERROR(err);
        HandleError();
        return;
    }

    assert(fd_ == chan_->fd());
    struct sockaddr_in addr = sock::GetLocalAddr(chan_->fd());
    std::string laddr = sock::ToIPPort(&addr);
    conn_fn_(chan_->fd(), laddr);
    timer_->Cancel();
    chan_->DisableAllEvent();
    chan_->Close();
    own_fd_ = false; // 将fd的所有权转移给TCPConn
    fd_ = INVALID_SOCKET;
    status_ = kConnected;
}

void Connector::HandleError() {
    assert(loop_->IsInLoopThread());
    int serrno = errno;
    LOG_ERROR << "status=" << StatusToString() << " fd=" << fd_ << " errno=" << serrno << " " << strerror(serrno);
    status_ = kDisconnected;

    if (chan_) {
        assert(fd_ > 0);
        chan_->DisableAllEvent();
        chan_->Close();
    }

    timer_->Cancel();

    if (EVUTIL_ERR_CONNECT_REFUSED(serrno)) {
        conn_fn_(-1, "");
    }

    if (fd_ > 0) {
        LOG_TRACE << "Connector::HandleError close(" << fd_ << ")";
        assert(own_fd_);
        EVUTIL_CLOSESOCKET(fd_);
        fd_ = INVALID_SOCKET;
    }

    if (owner_tcp_client_->auto_reconnect()) {
        LOG_INFO << "loop=" << loop_ << " auto reconnect in " << owner_tcp_client_->reconnect_interval().Seconds() << "s thread=" << std::this_thread::get_id();
        loop_->RunAfter(owner_tcp_client_->reconnect_interval(), std::bind(&Connector::Start, shared_from_this()));
    }
}

void Connector::OnConnectTimeout() {
    assert(status_ == kConnecting || status_ == kDNSResolving);
    EVUTIL_SET_SOCKET_ERROR(ETIMEDOUT);
    HandleError();
}

void Connector::OnDNSResolved(const std::vector <struct in_addr>& addrs) {
    if (addrs.empty()) {
        LOG_ERROR << "DNS Resolve failed. host=" << dns_resolver_->host();
        HandleError();
        return;
    }

    raddr_.sin_addr = addrs[0]; // TODO random index
    status_ = kDNSResolved;
    Connect();
}

std::string Connector::StatusToString() const {
    H_CASE_STRING_BIGIN(status_);
    H_CASE_STRING(kDisconnected);
    H_CASE_STRING(kDNSResolving);
    H_CASE_STRING(kDNSResolved);
    H_CASE_STRING(kConnecting);
    H_CASE_STRING(kConnected);
    H_CASE_STRING_END();
}
}
