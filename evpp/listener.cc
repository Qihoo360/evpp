#include "evpp/inner_pre.h"

#include "evpp/listener.h"
#include "evpp/event_loop.h"
#include "evpp/fd_channel.h"
#include "evpp/libevent_headers.h"
#include "evpp/sockets.h"

namespace evpp {
Listener::Listener(EventLoop* l, const std::string& addr)
    : loop_(l), addr_(addr) {}

Listener::~Listener() {
    LOG_TRACE << "Listener::~Listener fd=" << chan_->fd();
    chan_.reset();
    EVUTIL_CLOSESOCKET(fd_);
    fd_ = INVALID_SOCKET;
}

void Listener::Listen() {
    fd_ = sock::CreateNonblockingSocket();
    if (fd_ < 0) {
        int serrno = errno;
        LOG_FATAL << "Create a nonblocking socket failed " << strerror(serrno);
        return;
    }

    struct sockaddr_in addr = sock::ParseFromIPPort(addr_.data());
    int ret = ::bind(fd_, sock::sockaddr_cast(&addr), static_cast<socklen_t>(sizeof addr));
    if (ret < 0) {
        int serrno = errno;
        LOG_FATAL << "bind error :" << strerror(serrno);
    }

    ret = ::listen(fd_, SOMAXCONN);
    if (ret < 0) {
        int serrno = errno;
        LOG_FATAL << "Listen failed " << strerror(serrno);
    }
}

void Listener::Accept() {
    chan_.reset(new FdChannel(loop_, fd_, true, false));
    chan_->SetReadCallback(std::bind(&Listener::HandleAccept, this));
    loop_->RunInLoop(std::bind(&FdChannel::AttachToLoop, chan_.get()));
    LOG_INFO << "TCPServer is running at " << addr_;
}

void Listener::HandleAccept() {
    LOG_INFO << __FUNCTION__ << " New connection";
    assert(loop_->IsInLoopThread());
    struct sockaddr_storage ss;
    socklen_t addrlen = sizeof(ss);
    int nfd = -1;
    if ((nfd = ::accept(fd_, sock::sockaddr_cast(&ss), &addrlen)) == -1) {
        int serrno = errno;
        if (serrno != EAGAIN && serrno != EINTR) {
            LOG_WARN << __FUNCTION__ << " bad accept " << strerror(serrno);
        }
        return;
    }

    if (evutil_make_socket_nonblocking(nfd) < 0) {
        LOG_ERROR << "set nfd=" << nfd << " nonblocking failed.";
        EVUTIL_CLOSESOCKET(nfd);
        return;
    }

    sock::SetKeepAlive(nfd, true);

    std::string raddr = sock::ToIPPort(&ss);
    if (raddr.empty()) {
        EVUTIL_CLOSESOCKET(nfd);
        return;
    }

    LOG_INFO << "accepted a connection from " << raddr
             << ", listen fd=" << fd_
             << ", client fd=" << nfd;

    if (new_conn_fn_) {
        new_conn_fn_(nfd, raddr, sock::sockaddr_in_cast(&ss));
    }
}

void Listener::Stop() {
    assert(loop_->IsInLoopThread());
    chan_->DisableAllEvent();
    chan_->Close();
}
}
