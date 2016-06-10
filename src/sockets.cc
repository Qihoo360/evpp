#include "evpp/inner_pre.h"

#include "evpp/libevent_headers.h"
#include "evpp/sockets.h"

namespace evpp {
    EVPP_EXPORT std::string strerror(int e) {
#ifdef H_OS_WINDOWS
        LPVOID lpMsgBuf = NULL;
        ::FormatMessageA(
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            e,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPSTR)&lpMsgBuf,
            0, NULL);
        if (lpMsgBuf) {
            std::string s = (char*)lpMsgBuf;
            LocalFree(lpMsgBuf);
            return s;
        }
        return std::string();
#else
        char buf[1024] = {};
        return std::string(strerror_r(e, buf, sizeof buf));
#endif
    }


    int CreateNonblockingSocket() {
        int serrno = 0;
        int on = 1;

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

        ::setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (const char*)&on, sizeof(on));
        ::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&on, sizeof(on));
        return fd;
    out:
        EVUTIL_CLOSESOCKET(fd);
        return INVALID_SOCKET;
    }
}

#ifdef H_OS_WINDOWS
int readv(SOCKET sockfd, struct iovec *iov, int iovcnt) {
    DWORD readn = 0;
    DWORD flags = 0;
    if (::WSARecv(sockfd, iov, iovcnt, &readn, &flags, NULL, NULL) == 0) {
        return readn;
    }
    return -1;
}
#endif