#include "test_common.h"

#include <evpp/sockets.h>

TEST_UNIT(TestParseFromIPPort1) {
    std::string dd[] = {
        "192.168.0.6:99",
        "101.205.216.65:60931",
    };

    for (size_t i = 0; i < H_ARRAYSIZE(dd); i++) {
        struct sockaddr_storage ss;
        auto rc = evpp::sock::ParseFromIPPort(dd[i].data(), ss);
        H_TEST_ASSERT(rc);
        auto s = evpp::sock::ToIPPort(&ss);
        rc = s == dd[i];
        H_TEST_ASSERT(rc);
    }
}

TEST_UNIT(TestParseFromIPPort2) {
    std::string dd[] = {
        "5353.168.0.6",
        "5353.168.0.6:",
        "5353.168.0.6:99",
        "1011.205.216.65:60931",
    };

    for (size_t i = 0; i < H_ARRAYSIZE(dd); i++) {
        struct sockaddr_storage ss;
        auto rc = evpp::sock::ParseFromIPPort(dd[i].data(), ss);
        H_TEST_ASSERT(!rc);
        rc = evpp::sock::IsZeroAddress(&ss);
        H_TEST_ASSERT(rc);
    }
}

// TODO IPv6 test failed
#if 0
TEST_UNIT(TestParseFromIPPort3) {
    std::string dd[] = {
        "192.168.0.6:99",
        "101.205.216.65:60931",
        "[fe80::886a:49f3:20f3:add2:0]:80",
        "[fe80::c455:9298:85d2:f2b6:0]:8080",
    };

    for (size_t i = 0; i < H_ARRAYSIZE(dd); i++) {
        struct sockaddr_storage ss;
        auto rc = evpp::sock::ParseFromIPPort(dd[i].data(), ss);
        H_TEST_ASSERT(rc);
        auto s = evpp::sock::ToIPPort(&ss);
        rc = s == dd[i];
        H_TEST_ASSERT(rc);
    }
}

TEST_UNIT(TestParseFromIPPort4) {
    std::string dd[] = {
        "5353.168.0.6",
        "5353.168.0.6:",
        "5353.168.0.6:99",
        "1011.205.216.65:60931",
        "[fe80::886a:49f3:20f3:add2]",
        "[fe80::886a:49f3:20f3:add2]:",
        "fe80::886a:49f3:20f3:add2]:80",
        "[fe80::c455:9298:85d2:f2b6:8080",
    };

    for (size_t i = 0; i < H_ARRAYSIZE(dd); i++) {
        struct sockaddr_storage ss;
        auto rc = evpp::sock::ParseFromIPPort(dd[i].data(), ss);
        H_TEST_ASSERT(!rc);
        rc = evpp::sock::IsZeroAddress(&ss);
        H_TEST_ASSERT(rc);
    }
}
#endif


TEST_UNIT(TestSplitHostPort1) {
    struct {
        std::string addr;
        std::string host;
        int port;
    } dd[] = {
        {"192.168.0.6:99", "192.168.0.6", 99},
        { "101.205.216.65:60931", "101.205.216.65", 60931},
        {"[fe80::886a:49f3:20f3:add2]:80", "fe80::886a:49f3:20f3:add2", 80},
        {"[fe80::c455:9298:85d2:f2b6]:8080", "fe80::c455:9298:85d2:f2b6", 8080},
        {"fe80::886a:49f3:20f3:add2]:80", "fe80::886a:49f3:20f3:add2", 80}, // This is OK
    };


    for (size_t i = 0; i < H_ARRAYSIZE(dd); i++) {
        std::string host;
        int port;
        auto rc = evpp::sock::SplitHostPort(dd[i].addr.data(), host, port);
        H_TEST_ASSERT(rc);
        H_TEST_ASSERT(dd[i].host == host);
        H_TEST_ASSERT(dd[i].port == port);
    }
}

TEST_UNIT(TestSplitHostPort2) {
    struct {
        std::string addr;
        std::string host;
        int port;
    } dd[] = {
        {"[fe80::c455:9298:85d2:f2b6:8080", "fe80::c455:9298:85d2:f2b6", 8080} // This is not OK
    };

    for (size_t i = 0; i < H_ARRAYSIZE(dd); i++) {
        std::string host;
        int port;
        auto rc = evpp::sock::SplitHostPort(dd[i].addr.data(), host, port);
        H_TEST_ASSERT(!rc);
    }
}
