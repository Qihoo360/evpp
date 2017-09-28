#pragma once

#include "evpp/sys_addrinfo.h"
#include "evpp/sys_sockets.h"

#include <string.h>

namespace evpp {

class Duration;

EVPP_EXPORT std::string strerror(int e);

namespace sock {

EVPP_EXPORT evpp_socket_t CreateNonblockingSocket();
EVPP_EXPORT evpp_socket_t CreateUDPServer(int port);
EVPP_EXPORT void SetKeepAlive(evpp_socket_t fd, bool on);
EVPP_EXPORT void SetReuseAddr(evpp_socket_t fd);
EVPP_EXPORT void SetReusePort(evpp_socket_t fd);
EVPP_EXPORT void SetTCPNoDelay(evpp_socket_t fd, bool on);
EVPP_EXPORT void SetTimeout(evpp_socket_t fd, uint32_t timeout_ms);
EVPP_EXPORT void SetTimeout(evpp_socket_t fd, const Duration& timeout);
EVPP_EXPORT std::string ToIPPort(const struct sockaddr_storage* ss);
EVPP_EXPORT std::string ToIPPort(const struct sockaddr* ss);
EVPP_EXPORT std::string ToIPPort(const struct sockaddr_in* ss);
EVPP_EXPORT std::string ToIP(const struct sockaddr* ss);


// @brief Parse a literal network address and return an internet protocol family address
// @param[in] address - A network address of the form "host:port" or "[host]:port"
// @return bool - false if parse failed.
EVPP_EXPORT bool ParseFromIPPort(const char* address, struct sockaddr_storage& ss);

inline struct sockaddr_storage ParseFromIPPort(const char* address) {
    struct sockaddr_storage ss;
    bool rc = ParseFromIPPort(address, ss);
    if (rc) {
        return ss;
    } else {
        memset(&ss, 0, sizeof(ss));
        return ss;
    }
}

// @brief Splits a network address of the form "host:port" or "[host]:port"
//  into host and port. A literal address or host name for IPv6
// must be enclosed in square brackets, as in "[::1]:80" or "[ipv6-host]:80"
// @param[in] address - A network address of the form "host:port" or "[ipv6-host]:port"
// @param[out] host -
// @param[out] port - the port in local machine byte order
// @return bool - false if the network address is invalid format
EVPP_EXPORT bool SplitHostPort(const char* address, std::string& host, int& port);

EVPP_EXPORT struct sockaddr_storage GetLocalAddr(evpp_socket_t sockfd);

inline bool IsZeroAddress(const struct sockaddr_storage* ss) {
    const char* p = reinterpret_cast<const char*>(ss);
    for (size_t i = 0; i < sizeof(*ss); ++i) {
        if (p[i] != 0) {
            return false;
        }
    }
    return true;
}

template<typename To, typename From>
inline To implicit_cast(From const& f) {
    return f;
}

inline const struct sockaddr* sockaddr_cast(const struct sockaddr_in* addr) {
    return static_cast<const struct sockaddr*>(evpp::sock::implicit_cast<const void*>(addr));
}

inline struct sockaddr* sockaddr_cast(struct sockaddr_in* addr) {
    return static_cast<struct sockaddr*>(evpp::sock::implicit_cast<void*>(addr));
}

inline struct sockaddr* sockaddr_cast(struct sockaddr_storage* addr) {
    return static_cast<struct sockaddr*>(evpp::sock::implicit_cast<void*>(addr));
}

inline const struct sockaddr_in* sockaddr_in_cast(const struct sockaddr* addr) {
    return static_cast<const struct sockaddr_in*>(evpp::sock::implicit_cast<const void*>(addr));
}

inline struct sockaddr_in* sockaddr_in_cast(struct sockaddr* addr) {
    return static_cast<struct sockaddr_in*>(evpp::sock::implicit_cast<void*>(addr));
}

inline struct sockaddr_in* sockaddr_in_cast(struct sockaddr_storage* addr) {
    return static_cast<struct sockaddr_in*>(evpp::sock::implicit_cast<void*>(addr));
}

inline struct sockaddr_in6* sockaddr_in6_cast(struct sockaddr_storage* addr) {
    return static_cast<struct sockaddr_in6*>(evpp::sock::implicit_cast<void*>(addr));
}

inline const struct sockaddr_in* sockaddr_in_cast(const struct sockaddr_storage* addr) {
    return static_cast<const struct sockaddr_in*>(evpp::sock::implicit_cast<const void*>(addr));
}

inline const struct sockaddr_in6* sockaddr_in6_cast(const struct sockaddr_storage* addr) {
    return static_cast<const struct sockaddr_in6*>(evpp::sock::implicit_cast<const void*>(addr));
}

inline const struct sockaddr_storage* sockaddr_storage_cast(const struct sockaddr* addr) {
    return static_cast<const struct sockaddr_storage*>(evpp::sock::implicit_cast<const void*>(addr));
}

inline const struct sockaddr_storage* sockaddr_storage_cast(const struct sockaddr_in* addr) {
    return static_cast<const struct sockaddr_storage*>(evpp::sock::implicit_cast<const void*>(addr));
}

inline const struct sockaddr_storage* sockaddr_storage_cast(const struct sockaddr_in6* addr) {
    return static_cast<const struct sockaddr_storage*>(evpp::sock::implicit_cast<const void*>(addr));
}

}

}

#ifdef H_OS_WINDOWS
EVPP_EXPORT int readv(evpp_socket_t sockfd, struct iovec* iov, int iovcnt);
#endif
