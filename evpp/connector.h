#pragma once

#include <vector>

#include "evpp/inner_pre.h"
#include "evpp/duration.h"

namespace evpp {
class EventLoop;
class FdChannel;
class TimerEventWatcher;
class DNSResolver;
class TCPClient;
class EVPP_EXPORT Connector : public std::enable_shared_from_this<Connector> {
public:
    typedef std::function<void(evpp_socket_t sockfd, const std::string& /*local addr*/)> NewConnectionCallback;
    Connector(EventLoop* loop, TCPClient* client);
    ~Connector();
    void Start();
    void Cancel();
public:
    void SetNewConnectionCallback(NewConnectionCallback cb) {
        conn_fn_ = cb;
    }
    bool IsConnecting() const {
        return status_ == kConnecting;
    }
    bool IsConnected() const {
        return status_ == kConnected;
    }
    bool IsDisconnected() const {
        return status_ == kDisconnected;
    }
    int status() const {
        return status_;
    }
private:
    void Connect();
    void HandleWrite();
    void HandleError();
    void OnConnectTimeout();
    void OnDNSResolved(const std::vector <struct in_addr>& addrs);
    std::string StatusToString() const;
private:
    enum Status { kDisconnected, kDNSResolving, kDNSResolved, kConnecting, kConnected };
    Status status_;
    EventLoop* loop_;
    TCPClient* owner_tcp_client_;

    std::string remote_addr_; // host:port
    std::string remote_host_; // host
    int remote_port_ = 0; // port
    struct sockaddr_storage raddr_;

    Duration timeout_;

    evpp_socket_t fd_ = -1;

    // A flag indicate whether the Connector owns this fd.
    // If the Connector owns this fd, the Connector has responsibility to close this fd.
    bool own_fd_ = false;

    std::unique_ptr<FdChannel> chan_;
    std::unique_ptr<TimerEventWatcher> timer_;
    std::shared_ptr<DNSResolver> dns_resolver_;
    NewConnectionCallback conn_fn_;
};
}
