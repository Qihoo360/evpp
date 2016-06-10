#include "evpp/inner_pre.h"

#include "evpp/listener.h"
#include "evpp/event_loop.h"
#include "evpp/fd_channel.h"
#include "evpp/libevent_headers.h"
#include "evpp/sockets.h"

namespace evpp {
    Listener::Listener(EventLoop* l, const std::string& addr)
        : fd_(-1), loop_(l), addr_(addr) {}

    Listener::~Listener() {
        chan_.reset();
        EVUTIL_CLOSESOCKET(fd_);
        fd_ = INVALID_SOCKET;
    }

    void Listener::Listen() {
        fd_ = CreateNonblockingSocket();
        if (fd_ < 0) {
            return;
        }
        struct sockaddr_in addr = ParseFromIPPort(addr_.data());
        int ret = ::bind(fd_, sockaddr_cast(&addr), static_cast<socklen_t>(sizeof addr));
        int serrno = errno;
        if (ret < 0) {
            LOG_FATAL << "bind error :" << strerror(serrno);
        }

        ret = ::listen(fd_, SOMAXCONN);
        if (ret < 0) {
            serrno = errno;
            LOG_FATAL << "Listen failed " << strerror(serrno);
        }

        chan_ = xstd::shared_ptr<FdChannel>(new FdChannel(loop_, fd_, true, false));
        chan_->SetReadCallback(xstd::bind(&Listener::HandleAccept, this, xstd::placeholders::_1));
        loop_->RunInLoop(xstd::bind(&FdChannel::AttachToLoop, chan_.get()));
        LOG_INFO << "TCPServer is running at " << addr_;
    }

    void Listener::HandleAccept(base::Timestamp ts) {
        LOG_INFO << __FUNCTION__ << " New connection";

        struct sockaddr_storage ss;
        socklen_t addrlen = sizeof(ss);
        int nfd = -1;

        if ((nfd = ::accept(fd_, (struct sockaddr *)&ss, &addrlen)) == -1) {
            int serrno = errno;
            if (serrno != EAGAIN && serrno != EINTR)
                LOG_WARN << __FUNCTION__ << "bad accept " << strerror(serrno);
            return;
        }

        if (evutil_make_socket_nonblocking(nfd) < 0) {
            LOG_ERROR << "set nfd=" << nfd << " nonblocking failed.";
            EVUTIL_CLOSESOCKET(nfd);
            return;
        }

        int on = 1;
        ::setsockopt(nfd, SOL_SOCKET, SO_KEEPALIVE, (const char*)&on, sizeof(on));

        std::string raddr = GetRemoteAddr(ss);
        if (raddr.empty()) {
            EVUTIL_CLOSESOCKET(nfd);
            return;
        }

        LOG_INFO << "accepted one connection from " << raddr
            << ", listen fd=" << fd_
            << ", client fd=" << nfd;

        if (new_conn_fn_) {
            new_conn_fn_(nfd, raddr);
        }
    }

    std::string Listener::GetRemoteAddr(struct sockaddr_storage &ss) {
        std::string raddr;
        int port = 0;
        if (ss.ss_family == AF_INET) {
            struct sockaddr_in* addr4 = (sockaddr_in*)&ss;
            char buf[INET_ADDRSTRLEN] = {};
            const char* addr = ::inet_ntop(ss.ss_family, &addr4->sin_addr, buf, INET_ADDRSTRLEN);
            if (addr) {
                raddr = addr;
            }
            port = ::ntohs(addr4->sin_port);
        } else if (ss.ss_family == AF_INET6) {
            struct sockaddr_in6* addr6 = (sockaddr_in6*)&ss;
            char buf[INET6_ADDRSTRLEN] = {};
            const char* addr = ::inet_ntop(ss.ss_family, &addr6->sin6_addr, buf, INET6_ADDRSTRLEN);
            if (addr) {
                raddr = addr;
            }
            port = ::ntohs(addr6->sin6_port);
        } else {
            LOG_ERROR << "unknown socket family connected";
            return std::string();
        }

        if (!raddr.empty()) {
            char buf[16] = {};
            snprintf(buf, sizeof(buf), "%d", port);
            raddr.append(":", 1).append(buf);
        }

        return raddr;
    }

    void Listener::Stop() {
        loop_->AssertInLoopThread();
        chan_->DisableAllEvent();
        chan_->Close();
    }
}
