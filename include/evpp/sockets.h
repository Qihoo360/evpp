#pragma once

#include "evpp/inner_pre.h"

#include <errno.h>

#ifdef H_OS_WINDOWS
#include <ws2tcpip.h>
#include <WinSock2.h>
#include <io.h>
#include <ws2ipdef.h>

typedef int ssize_t;
#define iovec       _WSABUF
#define iov_base        buf
#define iov_len         len

#else
#include <arpa/inet.h>
#include <sys/uio.h>
#ifndef SOCKET
#	define SOCKET int		    /**< SOCKET definition */
#endif
#ifndef INVALID_SOCKET
#	define INVALID_SOCKET -1	/**< invalid socket definition */
#endif
#endif

#ifdef H_OS_WINDOWS
inline int readv(SOCKET sockfd, struct iovec *iov, int iovcnt) {
    DWORD readn = 0;
    DWORD flags = 0;
    if (::WSARecv(sockfd, iov, iovcnt, &readn, &flags, NULL, NULL) == 0) {
        return readn;
    }

    int serrno = errno;
    LOG_INFO << "WSAGetLastError=" << WSAGetLastError() << " errno=" << serrno;
    //errno = WSAGetLastError();
    return -1;
}
#endif


namespace evpp {
    template<typename To, typename From>
    inline To implicit_cast(From const &f) {
        return f;
    }

    inline const struct sockaddr* sockaddr_cast(const struct sockaddr_in* addr) {
        return static_cast<const struct sockaddr*>(implicit_cast<const void*>(addr));
    }

    inline struct sockaddr* sockaddr_cast(struct sockaddr_in* addr) {
        return static_cast<struct sockaddr*>(implicit_cast<void*>(addr));
    }

    inline const struct sockaddr_in* sockaddr_in_cast(const struct sockaddr* addr) {
        return static_cast<const struct sockaddr_in*>(implicit_cast<const void*>(addr));
    }

    inline struct sockaddr_in* sockaddr_in_cast(struct sockaddr* addr) {
        return static_cast<struct sockaddr_in*>(implicit_cast<void*>(addr));
    }

}
