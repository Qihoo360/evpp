#include "evpp/inner_pre.h"

#include "evpp/listener.h"
#include "evpp/event_loop.h"
#include "evpp/fd_channel.h"
#include "evpp/libevent_headers.h"
#include "evpp/sockets.h"

namespace evpp {

    static struct sockaddr_in fromIpPort(const char* address) {
        struct sockaddr_in addr;
        std::string a = address;
        size_t index = a.find_first_of(':');
        if (index == std::string::npos) {
            LOG_FATAL << "Address specified error [" << address << "]";
        }

        addr.sin_family = AF_INET;
        addr.sin_port = ::htons(::atoi(&a[index + 1]));
        a[index] = '\0';
        if (::inet_pton(AF_INET, a.data(), &addr.sin_addr) <= 0) {
            int serrno = errno;
            LOG_FATAL << "sockets::fromIpPort " << strerror(serrno);
        }
        return addr;
    }

    Listener::Listener(EventLoop* l, const std::string& addr)
        : loop_(l), addr_(addr), fd_(-1) {}

    Listener::~Listener() {
        chan_.reset();
        EVUTIL_CLOSESOCKET(fd_);
        fd_ = INVALID_SOCKET;
    }

    void Listener::Start() {
        fd_ = CreateNonblockingSocket();
        struct sockaddr_in addr = fromIpPort(addr_.data());
        int ret = ::bind(fd_, sockaddr_cast(&addr), static_cast<socklen_t>(sizeof addr));
        int serrno = errno;
        if (ret < 0) {
            LOG_FATAL << "bind error :" << strerror(serrno);
        }

        ret = ::listen(fd_, SOMAXCONN);
        if (ret < 0) {
            int serrno = errno;
            LOG_FATAL << "Listen failed " << strerror(serrno);
        }

        chan_ = xstd::shared_ptr<FdChannel>(new FdChannel(loop_, fd_, true, false));
        chan_->SetReadCallback(xstd::bind(&Listener::HandleAccept, this, xstd::placeholders::_1));
        loop_->RunInLoop(xstd::bind(&FdChannel::AttachToLoop, chan_.get()));
        LOG_INFO << "TCPServer is running at " << addr_;
    }


    int Listener::CreateNonblockingSocket() {
        int serrno;

        /* Create listen socket */
        int fd = socket(AF_INET, SOCK_STREAM, 0);
        serrno = errno;
        if (fd == -1) {
            LOG_FATAL << "socket error " << strerror(serrno);
            return INVALID_SOCKET;
        }

        if (evutil_make_socket_nonblocking(fd) < 0)
            goto out;

#ifndef WIN32
        if (fcntl(fd, F_SETFD, 1) == -1) {
            serrno = errno;
            LOG_FATAL << "fcntl(F_SETFD)" << strerror(serrno);
            goto out;
        }
#endif

        int on = 1;
        ::setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (const char*)&on, sizeof(on));
        ::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&on, sizeof(on));
        return fd;
    out:
        EVUTIL_CLOSESOCKET(fd);
        return INVALID_SOCKET;
    }

    void Listener::HandleAccept(base::Timestamp ts) {
        LOG_INFO << __FUNCTION__ << " New connections";

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

        std::string peer_addr = GetRemoteAddr(ss);
        if (peer_addr.empty()) {
            EVUTIL_CLOSESOCKET(nfd);
            return;
        }

        LOG_INFO << "accepted from : " << peer_addr
            << ", listen fd:" << fd_
            << ", client fd: " << nfd;

        if (new_conn_fn_) {
            new_conn_fn_(nfd, peer_addr);
        }
    }

    std::string Listener::GetRemoteAddr(struct sockaddr_storage &ss) {
        std::string peer_addr;
        int port = 0;
        if (ss.ss_family == AF_INET) {
            struct sockaddr_in* addr4 = (sockaddr_in*)&ss;
            char buf[INET_ADDRSTRLEN] = {};
            const char* addr = ::inet_ntop(ss.ss_family, &addr4->sin_addr, buf, INET_ADDRSTRLEN);
            if (addr) {
                peer_addr = addr;
            }
            port = ::ntohs(addr4->sin_port);
        } else if (ss.ss_family == AF_INET6) {
            struct sockaddr_in6* addr6 = (sockaddr_in6*)&ss;
            char buf[INET6_ADDRSTRLEN] = {};
            const char* addr = ::inet_ntop(ss.ss_family, &addr6->sin6_addr, buf, INET6_ADDRSTRLEN);
            if (addr) {
                peer_addr = addr;
            }
            port = ::ntohs(addr6->sin6_port);
        } else {
            LOG_ERROR << "unknown socket family connected";
            return std::string();
        }

        if (!peer_addr.empty()) {
            char buf[16] = {};
            snprintf(buf, sizeof(buf), "%d", port);
            peer_addr.append(":", 1).append(buf);
        }

        return peer_addr;
    }

}