#pragma once

#include "evpp/inner_pre.h"

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

    //     //TODO
    //     DWORD nError = WSAGetLastError();
    //     if (nError != WSA_IO_PENDING)
    //     {
    //         //set error state.
    //     }
    return -1;
}
#endif

