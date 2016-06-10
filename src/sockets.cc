#include "evpp/inner_pre.h"

#include "evpp/sockets.h"

namespace evpp {
    EVPP_EXPORT std::string strerror(int e) {
#ifdef H_OS_WINDOWS
//         LPVOID lpMsgBuf;
//         FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, 0, GetLastError(),
//                       MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
//                       (LPTSTR)&lpMsgBuf, 0, NULL);
//         std::string s = (char*)lpMsgBuf;
//         LocalFree(lpMsgBuf);

        LPVOID lpMsgBuf = NULL;
        FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, 0, e,
                      0,
                      (LPTSTR)&lpMsgBuf, 0, NULL);
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