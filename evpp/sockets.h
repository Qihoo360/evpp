#pragma once

#include "evpp/evpp_export.h"

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

    inline struct sockaddr_in* sockaddr_in_cast(struct sockaddr_storage* addr) {
        return static_cast<struct sockaddr_in*>(implicit_cast<void*>(addr));
    }

    inline struct sockaddr_in6* sockaddr_in6_cast(struct sockaddr_storage* addr) {
        return static_cast<struct sockaddr_in6*>(implicit_cast<void*>(addr));
    }

    inline const struct sockaddr_in* sockaddr_in_cast(const struct sockaddr_storage* addr) {
        return static_cast<const struct sockaddr_in*>(implicit_cast<const void*>(addr));
    }

    inline const struct sockaddr_in6* sockaddr_in6_cast(const struct sockaddr_storage* addr) {
        return static_cast<const struct sockaddr_in6*>(implicit_cast<const void*>(addr));
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
    EVPP_EXPORT void SetKeepAlive(int fd);
    EVPP_EXPORT struct sockaddr_in ParseFromIPPort(const char* address/*ip:port*/);
    EVPP_EXPORT struct sockaddr_in GetLocalAddr(int sockfd);
    EVPP_EXPORT std::string ToIPPort(const struct sockaddr_storage *ss);
}

#ifdef H_OS_WINDOWS
EVPP_EXPORT int readv(int sockfd, struct iovec *iov, int iovcnt);
#endif
