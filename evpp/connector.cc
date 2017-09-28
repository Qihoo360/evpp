#include "evpp/inner_pre.h"

#include "evpp/connector.h"
#include "evpp/event_loop.h"
#include "evpp/fd_channel.h"
#include "evpp/sockets.h"
#include "evpp/libevent.h"
#include "evpp/dns_resolver.h"
#include "evpp/tcp_client.h"

namespace evpp {
Connector::Connector(EventLoop* l, TCPClient* client)
    : status_(kDisconnected)
    , loop_(l)
    , owner_tcp_client_(client)
    , remote_addr_(client->remote_addr())
    , timeout_(client->connecting_timeout()) {
    DLOG_TRACE << "raddr=" << remote_addr_;
    if (sock::SplitHostPort(remote_addr_.data(), remote_host_, remote_port_)) {
        raddr_ = sock::ParseFromIPPort(remote_addr_.data());
    }
}

Connector::~Connector() {
    DLOG_TRACE;
    assert(loop_->IsInLoopThread());
    if (status_ == kDNSResolving) {
        assert(!chan_.get());
        assert(!dns_resolver_.get());
        assert(!timer_.get());
    } else if (!IsConnected()) {
        // A connected tcp-connection's sockfd has been transfered to TCPConn.
        // But the sockfd of unconnected tcp-connections need to be closed by myself.
        DLOG_TRACE << "close(" << chan_->fd() << ")";
        assert(own_fd_);
        assert(chan_->fd() == fd_);
        EVUTIL_CLOSESOCKET(fd_);
        fd_ = INVALID_SOCKET;
    }

    assert(fd_ < 0);
    chan_.reset();
}

void Connector::Start() {
    DLOG_TRACE << "Try to connect " << remote_addr_ << " status=" << StatusToString();
    assert(loop_->IsInLoopThread());

    timer_.reset(new TimerEventWatcher(loop_, std::bind(&Connector::OnConnectTimeout, shared_from_this()), timeout_));
    timer_->Init();
    timer_->AsyncWait();

    if (!sock::IsZeroAddress(&raddr_)) {
        Connect();
        return;
    }

    DLOG_TRACE << "The remote address " << remote_addr_ << " is a host, try to resolve its IP address.";
    status_ = kDNSResolving;
    auto f = std::bind(&Connector::OnDNSResolved, shared_from_this(), std::placeholders::_1);
    dns_resolver_ = std::make_shared<DNSResolver>(loop_, remote_host_, timeout_, f);
    dns_resolver_->Start();
}


void Connector::Cancel() {
    DLOG_TRACE << "Cancel to connect " << remote_addr_ << " status=" << StatusToString();
    assert(loop_->IsInLoopThread());
    if (dns_resolver_) {
        dns_resolver_->Cancel();
        dns_resolver_.reset();
    }

    assert(timer_);
    timer_->Cancel();
    timer_.reset();

    if (status_ == kDNSResolving) {
        assert(chan_.get() == nullptr);
        conn_fn_(-1, "");
    }

    if (chan_.get()) {
        assert(status_ != kDNSResolving);
        chan_->DisableAllEvent();
        chan_->Close();
    }
}

void Connector::Connect() {
    DLOG_TRACE << remote_addr_ << " status=" << StatusToString();
    assert(fd_ == INVALID_SOCKET);
    fd_ = sock::CreateNonblockingSocket();
    own_fd_ = true;
    assert(fd_ >= 0);
    const std::string& laddr = owner_tcp_client_->local_addr();
    if (!laddr.empty()) {
        struct sockaddr_storage ss = sock::ParseFromIPPort(laddr.data());
        struct sockaddr* addr = sock::sockaddr_cast(&ss);
        int rc = ::bind(fd_, addr, sizeof(*addr));
        if (rc != 0) {
            int serrno = errno;
            LOG_ERROR << "bind failed, errno=" << serrno << " " << strerror(serrno);
            HandleError();
            return;
        }
    }
    struct sockaddr* addr = sock::sockaddr_cast(&raddr_);
    int rc = ::connect(fd_, addr, sizeof(*addr));
    if (rc != 0) {
        int serrno = errno;
        if (!EVUTIL_ERR_CONNECT_RETRIABLE(serrno)) {
            HandleError();
            return;
        } else {
            // TODO how to do it
        }
    }

    status_ = kConnecting;

    chan_.reset(new FdChannel(loop_, fd_, false, true));
    DLOG_TRACE << "new FdChannel p=" << chan_.get() << " fd=" << chan_->fd();
    chan_->SetWriteCallback(std::bind(&Connector::HandleWrite, shared_from_this()));
    chan_->AttachToLoop();
}

void Connector::HandleWrite() {
    DLOG_TRACE << remote_addr_ << " status=" << StatusToString();
    if (status_ == kDisconnected) {
        // The connecting may be timeout, but the write event handler has been
        // dispatched in the EventLoop pending task queue, and next loop time the handle is invoked.
        // So we need to check the status whether it is at a kDisconnected
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
    struct sockaddr_storage addr = sock::GetLocalAddr(chan_->fd());
    std::string laddr = sock::ToIPPort(&addr);
    conn_fn_(chan_->fd(), laddr);
    timer_->Cancel();
    timer_.reset();
    chan_->DisableAllEvent();
    chan_->Close();
    own_fd_ = false; // Move the ownership of the fd to TCPConn
    fd_ = INVALID_SOCKET;
    status_ = kConnected;
}

void Connector::HandleError() {
    DLOG_TRACE << remote_addr_ << " status=" << StatusToString();
    assert(loop_->IsInLoopThread());
    int serrno = errno;

    // In this error handling method, we will invoke 'conn_fn_' callback function
    // to notify the user application layer in which the user maybe call TCPClient::Disconnect.
    // TCPClient::Disconnect may cause this Connector object desctruct.
    auto self = shared_from_this();

    LOG_ERROR << "this=" << this << " status=" << StatusToString() << " fd=" << fd_  << " use_count=" << self.use_count() << " errno=" << serrno << " " << strerror(serrno);

    status_ = kDisconnected;

    if (chan_) {
        assert(fd_ > 0);
        chan_->DisableAllEvent();
        chan_->Close();
    }

    // Avoid DNSResolver callback again when timeout
    if (dns_resolver_) {
        dns_resolver_->Cancel();
        dns_resolver_.reset();
    }

    timer_->Cancel();
    timer_.reset();

    // If the connection is refused or it will not try again,
    // We need to notify the user layer that the connection established failed.
    // Otherwise we will try to do reconnection silently.
    if (EVUTIL_ERR_CONNECT_REFUSED(serrno) || !owner_tcp_client_->auto_reconnect()) {
        conn_fn_(-1, "");
    }

    // Although TCPClient has a Reconnect() method to deal with automatically reconnection problem,
    // TCPClient's Reconnect() will be invoked when a established connection is broken down.
    //
    // But if we could not connect to the remote server at the very beginning,
    // the TCPClient's Reconnect() will never be triggled.
    // So Connector needs to do reconnection automatically itself.
    if (owner_tcp_client_->auto_reconnect()) {

        // We must close(fd) firstly and then we can do the reconnection.
        if (fd_ > 0) {
            DLOG_TRACE << "Connector::HandleError close(" << fd_ << ")";
            assert(own_fd_);
            EVUTIL_CLOSESOCKET(fd_);
            fd_ = INVALID_SOCKET;
        }

        DLOG_TRACE << "loop=" << loop_ << " auto reconnect in " << owner_tcp_client_->reconnect_interval().Seconds() << "s thread=" << std::this_thread::get_id();
        loop_->RunAfter(owner_tcp_client_->reconnect_interval(), std::bind(&Connector::Start, shared_from_this()));
    }
}

void Connector::OnConnectTimeout() {
    LOG_WARN << "this=" << this << " Connector::OnConnectTimeout status=" << StatusToString() << " fd=" << fd_ << " this=" << this;
    assert(status_ == kConnecting || status_ == kDNSResolving);
    EVUTIL_SET_SOCKET_ERROR(ETIMEDOUT);
    HandleError();
}

void Connector::OnDNSResolved(const std::vector <struct in_addr>& addrs) {
    DLOG_TRACE << "addrs.size=" << addrs.size() << " this=" << this;
    if (addrs.empty()) {
        LOG_ERROR << "this=" << this << " DNS Resolve failed. host=" << dns_resolver_->host();
        HandleError();
        return;
    }

    struct sockaddr_in* addr = sock::sockaddr_in_cast(&raddr_);
    addr->sin_family = AF_INET;
    addr->sin_port = htons(remote_port_);
    addr->sin_addr = addrs[0];
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
