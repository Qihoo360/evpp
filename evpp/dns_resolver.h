#pragma once

#include "evpp/inner_pre.h"
#include "evpp/duration.h"
#include "evpp/sys_addrinfo.h"

struct evdns_base;
struct evdns_getaddrinfo_request;
namespace evpp {
class EventLoop;
class TimerEventWatcher;
class EVPP_EXPORT DNSResolver : public std::enable_shared_from_this<DNSResolver> {
public:
    //TODO IPv6 DNS resolver
    typedef std::function<void(const std::vector<struct in_addr>& addrs)> Functor;

    DNSResolver(EventLoop* evloop, const std::string& host, Duration timeout, const Functor& f);
    ~DNSResolver();
    void Start();
    void Cancel();
    const std::string& host() const {
        return host_;
    }
private:
    void SyncDNSResolve();
    void AsyncDNSResolve();
    void AsyncWait();
    void OnTimeout();
    void OnCanceled();
    void ClearTimer();
    void OnResolved(int errcode, struct addrinfo* addr);
    void OnResolved();
    static void OnResolved(int errcode, struct addrinfo* addr, void* arg);
private:
    EventLoop* loop_;
    struct evdns_base* dnsbase_;
    struct evdns_getaddrinfo_request* dns_req_;
    std::string host_;
    Duration timeout_;
    Functor functor_;
    std::unique_ptr<TimerEventWatcher> timer_;
    std::vector<struct in_addr> addrs_;
};

}
