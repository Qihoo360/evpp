
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

    SyncDNSResolve();

    //TODO Implement asynchronous DNS resolve
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
}


void DNSResolver::AsyncDNSResolve() {
    /* Build the hints to tell getaddrinfo how to act. */
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC; /* v4 or v6 is fine. */
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP; /* We want a TCP socket */
    hints.ai_flags = AI_PASSIVE;    /* For wildcard IP address */

    //TODO
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
}
