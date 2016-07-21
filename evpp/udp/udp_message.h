#pragma once

#include "evpp/buffer.h"
#include "evpp/sys_sockets.h"
#include "evpp/sockets.h"

namespace evpp {

class EVPP_EXPORT UdpMessage : public Buffer {
public:
    UdpMessage(int fd, size_t buffer_size = 1472)
        : Buffer(buffer_size), sockfd_(fd) {
        memset(&remote_addr_, 0, sizeof(remote_addr_));
    }

    void set_remote_addr(const struct sockaddr& raddr);
    const struct sockaddr* remote_addr() const;
    struct sockaddr* mutable_remote_addr() { return sockaddr_cast(&remote_addr_); }

    int sockfd() const { return sockfd_; }
private:
    struct sockaddr_in remote_addr_;
    int sockfd_;
};
typedef std::shared_ptr<UdpMessage> UdpMessagePtr;

inline void UdpMessage::set_remote_addr(const struct sockaddr& raddr) {
    memcpy(&remote_addr_, &raddr, sizeof raddr);
}

inline const struct sockaddr* UdpMessage::remote_addr() const {
    return sockaddr_cast(&remote_addr_);
}

inline bool SendMessage(int fd, const struct sockaddr* addr, const char* d, size_t dlen) {
    int sentn = ::sendto(fd, d, dlen, 0, addr, sizeof(*addr));
    if (sentn != (int)dlen) {
        return false;
    }

    return true;
}

inline bool SendMessage(const UdpMessagePtr& msg) {
    return SendMessage(msg->sockfd(), msg->remote_addr(), msg->data(), msg->size());
}
}


