
#include "evpp/dns_resolver.h"
#include "evpp/event_loop.h"
#include "evpp/libevent_watcher.h"
#include "evpp/libevent_headers.h"

namespace evpp {
DNSResolver::DNSResolver(EventLoop* evloop, const std::string& h, Duration timeout, const Functor& f)
    : loop_(evloop), dnsbase_(nullptr), dns_req_(nullptr), host_(h), timeout_(timeout), functor_(f) {}

DNSResolver::~DNSResolver() {
    LOG_INFO << "DNSResolver::~DNSResolver tid=" << std::this_thread::get_id() << " this=" << this;

    assert(dnsbase_ == nullptr);

    if (dns_req_) {
        dns_req_ = nullptr;
    }
}

void DNSResolver::Start() {
    loop_->RunInLoop(std::bind(&DNSResolver::StartInLoop, this));
}

void DNSResolver::StartInLoop() {
    LOG_INFO << "DNSResolver::StartInLoop tid=" << std::this_thread::get_id() << " this=" << this;
    assert(loop_->IsInLoopThread());

#if LIBEVENT_VERSION_NUMBER >= 0x02001500
    AsyncDNSResolve();
#else
    SyncDNSResolve();
#endif
}

void DNSResolver::SyncDNSResolve() {
    /* Build the hints to tell getaddrinfo how to act. */
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC; /* v4 or v6 is fine. */
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP; /* We want a TCP socket */
    hints.ai_flags = AI_PASSIVE;    /* For wildcard IP address */

    /* Look up the hostname. */
    struct addrinfo* answer = nullptr;
    int err = getaddrinfo(host_.c_str(), nullptr, &hints, &answer);
    if (err != 0) {
        LOG_ERROR << "getaddrinfo failed. err=" << err << " " << gai_strerror(err);
    } else {
        for (struct addrinfo* rp = answer; rp != nullptr; rp = rp->ai_next) {
            struct sockaddr_in* a = reinterpret_cast<struct sockaddr_in*>(rp->ai_addr);

            if (a->sin_addr.s_addr == 0) {
                continue;
            }

            addrs_.push_back(a->sin_addr);
            LOG_TRACE << host_ << " resolved a ip=" << inet_ntoa(a->sin_addr);
        }
    }
    evutil_freeaddrinfo(answer);

    functor_(this->addrs_);
}

void DNSResolver::Cancel() {
    if (timer_) {
        loop_->RunInLoop(std::bind(&TimerEventWatcher::Cancel, timer_.get()));
    }
}

void DNSResolver::AsyncWait() {
    LOG_INFO << "DNSResolver::AsyncWait tid=" << std::this_thread::get_id() << " this=" << this;
    timer_.reset(new TimerEventWatcher(loop_, std::bind(&DNSResolver::OnTimeout, this), timeout_));
    timer_->SetCancelCallback(std::bind(&DNSResolver::OnCanceled, this));
    timer_->Init();
    timer_->AsyncWait();
}

void DNSResolver::OnTimeout() {
    LOG_INFO << "DNSResolver::OnTimeout tid=" << std::this_thread::get_id() << " this=" << this;
#if LIBEVENT_VERSION_NUMBER >= 0x02001500
    evdns_getaddrinfo_cancel(dns_req_);
#endif
    functor_(this->addrs_);
}

void DNSResolver::OnCanceled() {
    LOG_INFO << "DNSResolver::OnCanceled tid=" << std::this_thread::get_id() << " this=" << this;
#if LIBEVENT_VERSION_NUMBER >= 0x02001500
    evdns_getaddrinfo_cancel(dns_req_);
#endif
}


#if LIBEVENT_VERSION_NUMBER >= 0x02001500
void DNSResolver::AsyncDNSResolve() {
    /* Build the hints to tell getaddrinfo how to act. */
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC; /* v4 or v6 is fine. */
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP; /* We want a TCP socket */
    hints.ai_flags = AI_PASSIVE;    /* For wildcard IP address */

    dnsbase_ = evdns_base_new(loop_->event_base(), 1);
    dns_req_ = evdns_getaddrinfo(dnsbase_
                                 , host_.c_str()
                                 , nullptr /* no service name given */
                                 , &hints
                                 , &DNSResolver::OnResolved
                                 , this);
    assert(dnsbase_);
    assert(dns_req_);
    AsyncWait();
}

void DNSResolver::OnResolved(int errcode, struct addrinfo* addr) {
    if (errcode != 0) {
        if (errcode != EVUTIL_EAI_CANCEL) {
            timer_->Cancel();
            LOG_ERROR << "dns resolve failed, "
                      << ", error code: " << errcode
                      << ", error msg: " << evutil_gai_strerror(errcode);
        } else {
            LOG_WARN << "dns resolve cancel, may be timeout";
        }

        LOG_INFO << "delete dns ctx";
        evdns_base_free(dnsbase_, 0);
        dnsbase_ = nullptr;

        //No route to host
        //errno = EHOSTUNREACH;
        //OnError();
        functor_(this->addrs_);
        return;
    }


    if (addr == nullptr) {
        LOG_ERROR << "dns resolve error, addr can not be nullptr";

        LOG_INFO << "delete dns ctx";
        evdns_base_free(dnsbase_, 0);
        dnsbase_ = nullptr;

        //No route to host
        //errno = EHOSTUNREACH;
        //OnError();
        functor_(this->addrs_);
        return;
    }


    if (addr->ai_canonname) {
        LOG_INFO << "resolve canon namne: " << addr->ai_canonname;
    }

    for (struct addrinfo* rp = addr; rp != nullptr; rp = rp->ai_next) {
        struct sockaddr_in* a = sock::sockaddr_in_cast(rp->ai_addr);

        if (a->sin_addr.s_addr == 0) {
            continue;
        }

        addrs_.push_back(a->sin_addr);
        LOG_TRACE << host_ << " resolved a ip=" << inet_ntoa(a->sin_addr);
    }
    evutil_freeaddrinfo(addr);
    timer_->SetCancelCallback(TimerEventWatcher::Handler());
    timer_->Cancel();

    LOG_INFO << "delete dns ctx";
    evdns_base_free(dnsbase_, 0);
    dnsbase_ = nullptr;
    functor_(this->addrs_);
}

void DNSResolver::OnResolved(int errcode, struct addrinfo* addr, void* arg) {
    DNSResolver* t = (DNSResolver*)arg;
    t->OnResolved(errcode, addr);
}
#endif

}
