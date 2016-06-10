#pragma once

#include "evpp/inner_pre.h"


#ifdef H_OS_WINDOWS

#include <windows.h>
#define errno WSAGetLastError()

#include <ws2tcpip.h>
#include <WinSock2.h>
#include <io.h>
#include <ws2ipdef.h>

typedef int ssize_t;
#define iovec _WSABUF
#define iov_base buf
#define iov_len  len

#else
#include <unistd.h>
#include <fcntl.h>
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
EVPP_EXPORT int readv(SOCKET sockfd, struct iovec *iov, int iovcnt);



/*
* Windows Sockets errors redefined as regular Berkeley error constants.
* Copied from winsock.h
*/
#if 1
#define EWOULDBLOCK             WSAEWOULDBLOCK
#define EINPROGRESS             WSAEINPROGRESS
#define EALREADY                WSAEALREADY
#define ENOTSOCK                WSAENOTSOCK
#define EDESTADDRREQ            WSAEDESTADDRREQ
#define EMSGSIZE                WSAEMSGSIZE
#define EPROTOTYPE              WSAEPROTOTYPE
#define ENOPROTOOPT             WSAENOPROTOOPT
#define EPROTONOSUPPORT         WSAEPROTONOSUPPORT
#define ESOCKTNOSUPPORT         WSAESOCKTNOSUPPORT
#define EOPNOTSUPP              WSAEOPNOTSUPP
#define EPFNOSUPPORT            WSAEPFNOSUPPORT
#define EAFNOSUPPORT            WSAEAFNOSUPPORT
#define EADDRINUSE              WSAEADDRINUSE
#define EADDRNOTAVAIL           WSAEADDRNOTAVAIL
#define ENETDOWN                WSAENETDOWN
#define ENETUNREACH             WSAENETUNREACH
#define ENETRESET               WSAENETRESET
#define ECONNABORTED            WSAECONNABORTED
#define ECONNRESET              WSAECONNRESET
#define ENOBUFS                 WSAENOBUFS
#define EISCONN                 WSAEISCONN
#define ENOTCONN                WSAENOTCONN
#define ESHUTDOWN               WSAESHUTDOWN
#define ETOOMANYREFS            WSAETOOMANYREFS
#define ETIMEDOUT               WSAETIMEDOUT
#define ECONNREFUSED            WSAECONNREFUSED
#define ELOOP                   WSAELOOP
#define ENAMETOOLONG            WSAENAMETOOLONG
#define EHOSTDOWN               WSAEHOSTDOWN
#define EHOSTUNREACH            WSAEHOSTUNREACH
#define ENOTEMPTY               WSAENOTEMPTY
#define EPROCLIM                WSAEPROCLIM
#define EUSERS                  WSAEUSERS
#define EDQUOT                  WSAEDQUOT
#define ESTALE                  WSAESTALE
#define EREMOTE                 WSAEREMOTE

#define EAGAIN EWOULDBLOCK // Added by @weizili at 20160610
#endif

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

    inline const struct sockaddr_storage* sockaddr_storage_cast(const struct sockaddr* addr) {
        return static_cast<const struct sockaddr_storage*>(implicit_cast<const void*>(addr));
    }

    inline const struct sockaddr_storage* sockaddr_storage_cast(const struct sockaddr_in* addr) {
        return static_cast<const struct sockaddr_storage*>(implicit_cast<const void*>(addr));
    }

    inline const struct sockaddr_storage* sockaddr_storage_cast(const struct sockaddr_in6* addr) {
        return static_cast<const struct sockaddr_storage*>(implicit_cast<const void*>(addr));
    }

    EVPP_EXPORT std::string strerror(int e);
    EVPP_EXPORT int CreateNonblockingSocket();

    EVPP_EXPORT struct sockaddr_in ParseFromIPPort(const char* address/*ip:port*/);
    EVPP_EXPORT struct sockaddr_in GetLocalAddr(int sockfd);
    EVPP_EXPORT std::string ToIPPort(const struct sockaddr_storage *ss);
}
