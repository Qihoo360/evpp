#pragma once

#include "evpp/inner_pre.h"
#include "evpp/duration.h"
#include "evpp/sys_addrinfo.h"

namespace evpp {
class EventLoop;
class TimerEventWatcher;
class EVPP_EXPORT DNSResolver {
public:
    typedef std::function<void(const std::vector <struct in_addr>& addrs)> Functor;
    DNSResolver(EventLoop* evloop, const std::string& host, Duration timeout, const Functor& f);
    ~DNSResolver();
    void Start();
    void Cancel();
private:
    void StartInLoop();
    void AsyncWait(Duration timeout);
    void OnTimeout();
    void OnCanceled();
    void OnResolved(int result, char type, int count, int ttl, void* addresses);
    static void OnResolved(int result, char type, int count, int ttl, void* addresses, void* arg);
private:
    EventLoop* loop_;
    std::string host_;
    Duration timeout_;
    Functor functor_;
    TimerEventWatcher* timer_;
    std::vector <struct in_addr> addrs_;
};
}
