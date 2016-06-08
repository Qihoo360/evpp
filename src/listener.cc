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

        chan_ = xstd::shared_ptr<FdChannel>(new FdChannel(loop_->event_base(), fd_, true, false));
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
}