
#include "evpp/dns_resolver.h"
#include "evpp/event_loop.h"
#include "evpp/libevent_watcher.h"
#include "evpp/libevent_headers.h"

namespace evpp {
DNSResolver::DNSResolver(EventLoop* evloop, const std::string& host, Duration timeout, const Functor& f)
    : loop_(evloop), host_(host), timeout_(timeout), functor_(f), timer_(NULL) {}

DNSResolver::~DNSResolver() {
    LOG_INFO << "DNSResolver::~DNSResolver tid=" << std::this_thread::get_id() << " this=" << this;

    if (timer_) {
        delete timer_;
        timer_ = NULL;
    }
}

void DNSResolver::Start() {
    loop_->RunInLoop(std::bind(&DNSResolver::StartInLoop, this));
}

void DNSResolver::StartInLoop() {
    LOG_INFO << "DNSResolver::StartInLoop tid=" << std::this_thread::get_id() << " this=" << this;
    loop_->AssertInLoopThread();


    /* Build the hints to tell getaddrinfo how to act. */
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC; /* v4 or v6 is fine. */
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP; /* We want a TCP socket */
    hints.ai_flags = AI_PASSIVE;    /* For wildcard IP address */
    hints.ai_protocol = 0;          /* Any protocol */
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    /* Look up the hostname. */
    struct addrinfo* answer = NULL;
    int err = getaddrinfo(host_.c_str(), NULL, &hints, &answer);

    if (err != 0) {
        LOG_ERROR << "getaddrinfo failed. err=" << err << " " << gai_strerror(err);
    } else {
        for (struct addrinfo* rp = answer; rp != NULL; rp = rp->ai_next) {
            struct sockaddr_in* a = (struct sockaddr_in*)(rp->ai_addr);

            if (a->sin_addr.s_addr == 0) {
                continue;
            }

            addrs_.push_back(a->sin_addr);
            LOG_TRACE << host_ << " resolved a ip=" << inet_ntoa(a->sin_addr);
        }
    }

    functor_(this->addrs_);
    //TODO Implement asynchronous DNS resolve
}

void DNSResolver::Cancel() {
    if (timer_) {
        loop_->RunInLoop(std::bind(&TimerEventWatcher::Cancel, timer_));
    }
}

void DNSResolver::AsyncWait(Duration timeout) {
    LOG_INFO << "DNSResolver::AsyncWait tid=" << std::this_thread::get_id() << " this=" << this;
    timer_ = new TimerEventWatcher(loop_, std::bind(&DNSResolver::OnTimeout, this), timeout_);
    timer_->set_cancel_callback(std::bind(&DNSResolver::OnCanceled, this));
    timer_->Init();
    timer_->AsyncWait();
}

void DNSResolver::OnTimeout() {
    LOG_INFO << "DNSResolver::OnTimeout tid=" << std::this_thread::get_id() << " this=" << this;
    functor_(this->addrs_);
}

void DNSResolver::OnCanceled() {
    LOG_INFO << "DNSResolver::OnCanceled tid=" << std::this_thread::get_id() << " this=" << this;
}

/**
* The callback that contains the results from a lookup.
* - type is either DNS_IPv4_A or DNS_PTR or DNS_IPv6_AAAA
* - count contains the number of addresses of form type
* - ttl is the number of seconds the resolution may be cached for.
* - addresses needs to be cast according to type
*/
void DNSResolver::OnResolved(int result, char type, int count, int ttl, void* addresses) {
    int dns_ok = 0;
    int dns_err = 0;

    if (result == DNS_ERR_TIMEOUT) {
        LOG_WARN << "DNS Resolved [Timed out]";
        dns_err = result;
        goto out;
    }

    if (result != DNS_ERR_NONE) {
        LOG_WARN << "DNS Resolved Error code " << result;
        goto out;
    }

    LOG_TRACE << "type=" << type << " count=" << count << " ttl=" << ttl;

    switch (type) {
    case DNS_IPv6_AAAA: {
#if defined(HAVE_STRUCT_IN6_ADDR) && defined(HAVE_INET_NTOP) && defined(INET6_ADDRSTRLEN)
        struct in6_addr* in6_addrs = addresses;
        char buf[INET6_ADDRSTRLEN + 1];
        int i;

        /* a resolution that's not valid does not help */
        if (ttl < 0) {
            goto out;
        }

        for (i = 0; i < count; ++i) {
            const char* b = inet_ntop(AF_INET6, &in6_addrs[i], buf, sizeof(buf));

            if (b) {
                fprintf(stderr, "%s ", b);
            } else {
                fprintf(stderr, "%s ", strerror(errno));
            }
        }

#endif
        break;
    }

    case DNS_IPv4_A: {
        struct in_addr* in_addrs = (struct in_addr*)addresses;

        /* a resolution that's not valid does not help */
        if (ttl < 0) {
            goto out;
        }

        for (int i = 0; i < count; ++i) {
            this->addrs_.push_back(in_addrs[i]);
            fprintf(stderr, "%s ", inet_ntoa(in_addrs[i]));
        }

        break;
    }

    case DNS_PTR:

        /* may get at most one PTR */
        if (count != 1) {
            goto out;
        }

        fprintf(stderr, "%s ", *(char**)addresses);
        break;

    default:
        goto out;
    }

    dns_ok = type;
    (void)dns_ok;
    (void)dns_err;
out:
    this->functor_(this->addrs_);
    evdns_shutdown(0);
}

void DNSResolver::OnResolved(int result, char type, int count, int ttl, void* addresses, void* arg) {
    DNSResolver* thiz = (DNSResolver*)arg;
    thiz->OnResolved(result, type, count, ttl, addresses);
}

}
