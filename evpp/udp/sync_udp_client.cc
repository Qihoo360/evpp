#include "evpp/inner_pre.h"

#include "sync_udp_client.h"
#include "evpp/libevent_headers.h"
#include "evpp/sockets.h"

namespace evpp {
namespace udp {
namespace sync {
UdpClient::UdpClient() {
    sockfd_ = INVALID_SOCKET;
    memset(&remote_addr_, 0, sizeof(remote_addr_));
}

UdpClient::~UdpClient(void) {
    Close();
}

bool UdpClient::Connect(const struct sockaddr_in& addr) {
    memcpy(&remote_addr_, &addr, sizeof(remote_addr_));
    return Connect();
}

bool UdpClient::Connect(const char* host, int port) {
    char buf[32] = {};
    snprintf(buf, sizeof buf, "%s:%d", host, port);
    return Connect(buf);
}

bool UdpClient::Connect(const char* addr/*host:port*/) {
    struct sockaddr_in raddr = ParseFromIPPort(addr);
    return Connect(raddr);
}

bool UdpClient::Connect(const struct sockaddr& addr) {
    memcpy(&remote_addr_, &addr, sizeof(remote_addr_));
    return Connect();
}

bool UdpClient::Connect() {
    sockfd_ = socket(AF_INET, SOCK_DGRAM, 0);
    SetReuseAddr(sockfd_);

    socklen_t addrlen = sizeof(remote_addr_);
    int ret = ::connect(sockfd_, (struct sockaddr*)&remote_addr_, addrlen);

    if (ret != 0) {
        Close();
        struct sockaddr_in *paddr = (struct sockaddr_in*)&remote_addr_;
        LOG_ERROR << "Failed to connect to remote IP="
            << inet_ntoa(paddr->sin_addr)
            << ", port=" << ntohs(paddr->sin_port)
            << ", errno=" << errno << " " << strerror(errno);
        return false;
    }

    return true;
}


void UdpClient::Close() {
    EVUTIL_CLOSESOCKET(sockfd_);
}


std::string UdpClient::DoRequest(const std::string& data, uint32_t timeout_ms) {
    if (!Send(data)) {
        LOG_ERROR << "sent failed, errno=" << errno << " , dlen=" << data.size();
        return "";
    }

    SetTimeout(sockfd_, timeout_ms);

    size_t nBufSize = 1472; // The UDP max payload size
    UdpMessagePtr msg(new UdpMessage(sockfd_, nBufSize));
    socklen_t m_nAddrLen = sizeof(remote_addr_);
    int readn = ::recvfrom(sockfd_, msg->WriteBegin(), nBufSize, 0, msg->mutable_remote_addr(), &m_nAddrLen);
    int err = errno;
    if (readn == 0) {
        //TODO ERROR process
        LOG_ERROR << "errno=" << err << " " << strerror(err) << " recvfrom return 0";
    } else if (readn > 0) {
        msg->WriteBytes(readn);
        return std::string(msg->data(), msg->size());
    } else {
        LOG_ERROR << "errno=" << err << " " << strerror(err) << " recvfrom return -1";
    }

    return "";
}

std::string UdpClient::DoRequest(const std::string& remote_ip, int port, const std::string& udp_package_data, uint32_t timeout_ms) {
    UdpClient c;
    if (!c.Connect(remote_ip.data(), port)) {
        return "";
    }

    return c.DoRequest(udp_package_data, timeout_ms);
}

bool UdpClient::Send(const char* msg, size_t len) {
    int sentn = ::sendto(sockfd(),
                            msg,
                            len, 0, &remote_addr_, sizeof(remote_addr_));
    return sentn > 0;
}

bool UdpClient::Send(const std::string& msg) {
    return Send(msg.data(), msg.size());
}

bool UdpClient::Send(const std::string& msg, const struct sockaddr_in& addr) {
    return UdpClient::Send(msg.data(), msg.size(), addr);
}


bool UdpClient::Send(const char* msg, size_t len, const struct sockaddr_in& addr) {
    UdpClient c;
    if (!c.Connect(addr)) {
        return "";
    }

    return c.Send(msg, len);
}

bool UdpClient::Send(const UdpMessagePtr& msg) {
    return UdpClient::Send(msg->data(), msg->size(), *(const struct sockaddr_in*)(msg->remote_addr()));
}
}
}
}


