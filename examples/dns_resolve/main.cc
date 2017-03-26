#include <evpp/event_loop.h>
#include <evpp/dns_resolver.h>
#include <evpp/sockets.h>

#ifdef WIN32
#include "examples/winmain-inl.h"
#endif

int main(int argc, char* argv[]) {
    std::string host = "www.so.com";
    if (argc > 1) {
        host = argv[1];
    }

    evpp::EventLoop loop;

    auto fn_resolved = [&loop, &host](const std::vector <struct in_addr>& addrs) {
        LOG_INFO << "Entering fn_resolved";
        for (auto addr : addrs) {
            struct sockaddr_in saddr;
            memset(&saddr, 0, sizeof(saddr));
            saddr.sin_addr = addr;
            LOG_INFO << "DNS resolved " << host << " ip " << evpp::sock::ToIP(evpp::sock::sockaddr_cast(&saddr));
        }

        loop.RunAfter(evpp::Duration(0.5), [&loop]() { loop.Stop(); });
    };

    std::shared_ptr<evpp::DNSResolver> dns_resolver(new evpp::DNSResolver(&loop , host, evpp::Duration(1.0), fn_resolved));
    dns_resolver->Start();
    loop.Run();

    return 0;
}
