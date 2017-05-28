#pragma once


#include "platform_config.h"

#ifdef H_OS_WINDOWS


#include <string> // avoid compiling failed because of 'errno' redefined as 'WSAGetLastError()'
#define errno WSAGetLastError()
#include <windows.h>

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
#   define SOCKET int           /**< SOCKET definition */
#endif
#ifndef INVALID_SOCKET
#   define INVALID_SOCKET -1    /**< invalid socket definition */
#endif
#endif

#ifdef H_OS_WINDOWS

/*
 * Windows Sockets errors redefined as regular Berkeley error constants.
 * Copied from winsock.h
 */
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

#define gai_strerror gai_strerrorA

#endif // end of H_OS_WINDOWS

#if (defined(H_OS_WINDOWS) || defined(H_OS_MACOSX))

#ifndef HAVE_MSG_NOSIGNAL
#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif
#endif

#ifndef HAVE_MSG_DONTWAIT
#ifndef MSG_DONTWAIT
#define MSG_DONTWAIT 0
#endif
#endif

#endif


// Copied from libevent2.0/util-internal.h
#ifdef H_OS_WINDOWS

#define EVUTIL_ERR_RW_RETRIABLE(e)                  \
    ((e) == WSAEWOULDBLOCK || \
     (e) == WSAETIMEDOUT || \
     (e) == WSAEINTR)

#define EVUTIL_ERR_CONNECT_RETRIABLE(e)                 \
    ((e) == WSAEWOULDBLOCK || \
     (e) == WSAEINTR || \
     (e) == WSAEINPROGRESS || \
     (e) == WSAEINVAL)

#define EVUTIL_ERR_ACCEPT_RETRIABLE(e)          \
    EVUTIL_ERR_RW_RETRIABLE(e)

#define EVUTIL_ERR_CONNECT_REFUSED(e)                   \
    ((e) == WSAECONNREFUSED)

#else

/* True iff e is an error that means a read/write operation can be retried. */
#define EVUTIL_ERR_RW_RETRIABLE(e)              \
    ((e) == EINTR || (e) == EAGAIN)
/* True iff e is an error that means an connect can be retried. */
#define EVUTIL_ERR_CONNECT_RETRIABLE(e)         \
    ((e) == EINTR || (e) == EINPROGRESS)
/* True iff e is an error that means a accept can be retried. */
#define EVUTIL_ERR_ACCEPT_RETRIABLE(e)          \
    ((e) == EINTR || (e) == EAGAIN || (e) == ECONNABORTED)

/* True iff e is an error that means the connection was refused */
#define EVUTIL_ERR_CONNECT_REFUSED(e)                   \
    ((e) == ECONNREFUSED)

#endif


#ifdef H_OS_WINDOWS
#define evpp_socket_t intptr_t
#else
#define evpp_socket_t int
#endif

#define signal_number_t evpp_socket_t