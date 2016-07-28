#include "evpp/inner_pre.h"

#include "evpp/connector.h"
#include "evpp/event_loop.h"
#include "evpp/fd_channel.h"
#include "evpp/sockets.h"
#include "evpp/libevent_headers.h"
#include "evpp/dns_resolver.h"

namespace evpp {
Connector::Connector(EventLoop* l, const std::string& raddr, Duration timeout)
    : status_(kDisconnected), loop_(l), remote_addr_(raddr), timeout_(timeout) {
    LOG_INFO << "Connector::Connector this=" << this << " raddr=" << raddr;
    raddr_ = sock::ParseFromIPPort(remote_addr_.data());
}

Connector::~Connector() {
    loop_->AssertInLoopThread();

    if (!IsConnected()) {
        // A connected tcp-connection's sockfd has been transfered to TCPConn.
        // But the sockfd of unconnected tcp-connections need to be closed by myself.
        LOG_TRACE << "Connector::~Connector close(" << chan_->fd() << ")";
        assert(chan_->fd() > 0);
        EVUTIL_CLOSESOCKET(chan_->fd());
        chan_->SetInvalidSocket();
    }

    assert(chan_->fd() < 0);
    chan_.reset();
}

void Connector::Start() {
    LOG_INFO << "Try to connect " << remote_addr_ << " status=" << StatusToString();
    loop_->AssertInLoopThread();

    timer_.reset(new TimerEventWatcher(loop_, std::bind(&Connector::OnConnectTimeout, this), timeout_));
    timer_->Init();
    timer_->AsyncWait();

    if (raddr_.sin_addr.s_addr == 0) {
        status_ = kDNSResolving;
        auto index = remote_addr_.rfind(':');
        assert(index != std::string::npos);
        auto host = std::string(remote_addr_.data(), index);
        dns_resolver_.reset(new DNSResolver(loop_, host, timeout_, std::bind(&Connector::OnDNSResolved, this, std::placeholders::_1)));
        dns_resolver_->Start();
        return;
    }

    Connect();
}


void Connector::Cancel() {
    LOG_INFO << "Cancel to connect " << remote_addr_ << " status=" << StatusToString();
    loop_->AssertInLoopThread();
    if (dns_resolver_) {
        dns_resolver_->Cancel();
    }

    assert(timer_);
    timer_->Cancel();
    chan_->DisableAllEvent();
    chan_->Close();
}

void Connector::Connect() {
    int fd = sock::CreateNonblockingSocket();
    assert(fd >= 0);
    int rc = ::connect(fd, sock::sockaddr_cast(&raddr_), sizeof(raddr_));
    if (rc != 0) {
        int serrno = errno;
        if (!EVUTIL_ERR_CONNECT_RETRIABLE(serrno)) {
            HandleError();
            EVUTIL_CLOSESOCKET(fd);
            return;
        }
    }

    status_ = kConnecting;

    chan_.reset(new FdChannel(loop_, fd, false, true));
    LOG_TRACE << "this=" << this << " new FdChannel p=" << chan_.get() << " fd=" << chan_->fd();
    chan_->SetWriteCallback(std::bind(&Connector::HandleWrite, this));
    chan_->AttachToLoop();
}

void Connector::HandleWrite() {
    if (status_ == kDisconnected) {
        // 这里有可能是超时了，但回调时间已经派发到队列中，后面才调用。
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

    struct sockaddr_in addr = sock::GetLocalAddr(chan_->fd());
    std::string laddr = sock::ToIPPort(&addr);
    conn_fn_(chan_->fd(), laddr);
    timer_->Cancel();
    chan_->DisableAllEvent();
    chan_->Close();
    chan_->SetInvalidSocket();
    status_ = kConnected;
}

void Connector::HandleError() {
    int serrno = errno;
    LOG_ERROR << "status=" << StatusToString() << " errno=" << serrno << " " << strerror(serrno);
    status_ = kDisconnected;

    if (chan_) {
        chan_->DisableAllEvent();
        chan_->Close();
    }

    timer_->Cancel();

    if (EVUTIL_ERR_CONNECT_REFUSED(serrno)) {
        conn_fn_(-1, "");
    } else {
        loop_->RunAfter(1000, std::bind(&Connector::Start, this));//TODO Add retry times.
    }
}

void Connector::OnConnectTimeout() {
    assert(status_ == kConnecting || status_ == kDNSResolving);
    EVUTIL_SET_SOCKET_ERROR(ETIMEDOUT);
    HandleError();
}


void Connector::OnDNSResolved(const std::vector <struct in_addr>& addrs) {
    if (addrs.empty()) {
        LOG_ERROR << "DNS Resolve failed. host=" << remote_addr_;
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
