#include "test_common.h"

#include <evpp/event_loop.h>
#include <evpp/event_loop_thread.h>
#include <evpp/dns_resolver.h>
#include <atomic>

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
        std::shared_ptr<evpp::DNSResolver> dns_resolver(new evpp::DNSResolver(t->event_loop(), "www.so.com", evpp::Duration(1.0), fn_resolved));
        dns_resolver->Start();

        while (!resolved) {
            usleep(1);
        }

        auto fn_deleter = [&deleted, dns_resolver]() {
            LOG_INFO << "Entering fn_deleter";
            deleted = true;
        };

        t->event_loop()->QueueInLoop(fn_deleter);
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
