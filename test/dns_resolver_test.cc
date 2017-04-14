#include "test_common.h"

#include <evpp/event_loop.h>
#include <evpp/event_loop_thread.h>
#include <evpp/dns_resolver.h>

TEST_UNIT(testDNSResolver) {
    for (int i = 0; i < 6; i++) {
        bool resolved = false;
        bool deleted = false;
        auto fn_resolved = [&resolved](const std::vector <struct in_addr>& addrs) {
            LOG_INFO << "Entering fn_resolved";
            resolved = true;
        };

        evpp::Duration delay(double(3.0)); // 3s
        std::unique_ptr<evpp::EventLoopThread> t(new evpp::EventLoopThread);
        t->Start(true);
        std::shared_ptr<evpp::DNSResolver> dns_resolver(
            new evpp::DNSResolver(t->loop(), "www.so.com", delay, fn_resolved));
        dns_resolver->Start();

        while (!resolved) {
            usleep(1);
        }

        auto fn_deleter = [&deleted, dns_resolver]() {
            LOG_INFO << "Entering fn_deleter";
            deleted = true;
        };

        t->loop()->QueueInLoop(fn_deleter);
        dns_resolver.reset();
        while (!deleted) {
            usleep(1);
        }

        t->Stop(true);
        t.reset();
        if (evpp::GetActiveEventCount() != 0) {
            H_TEST_ASSERT(evpp::GetActiveEventCount() == 0);
        }
    }
}

TEST_UNIT(testDNSResolverTimeout) {
    for (int i = 0; i < 6; i++) {
        std::atomic<int> resolved(0);
        bool deleted = false;
        auto fn_resolved = [&resolved](const std::vector <struct in_addr>& addrs) {
            LOG_INFO << "Entering fn_resolved addrs.size=" << addrs.size();
            resolved.fetch_add(1);
        };

        // 1us to make it timeout immediately
        evpp::Duration delay(double(0.000001));
        std::unique_ptr<evpp::EventLoopThread> t(new evpp::EventLoopThread);
        t->Start(true);
        std::shared_ptr<evpp::DNSResolver> dns_resolver(
            new evpp::DNSResolver(t->loop(),
                                  "wwwwwww.en.cppreference.com",
                                  delay,
                                  fn_resolved));
        dns_resolver->Start();

        while (!resolved) {
            usleep(1);
        }

        auto fn_deleter = [&deleted, dns_resolver]() {
            LOG_INFO << "Entering fn_deleter";
            deleted = true;
        };

        t->loop()->QueueInLoop(fn_deleter);
        dns_resolver.reset();
        while (!deleted) {
            usleep(1);
        }

        t->loop()->RunAfter(evpp::Duration(0.05), [loop = t.get()]() { loop->Stop(); });
        while (!t->IsStopped()) {
            usleep(1);
        }
        t.reset();
        assert(resolved.load() == 1);
        if (evpp::GetActiveEventCount() != 0) {
            H_TEST_ASSERT(evpp::GetActiveEventCount() == 0);
        }
    }
}
