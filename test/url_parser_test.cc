#include "test_common.h"

#include <evpp/httpc/url_parser.h>

TEST_UNIT(testURLParser) {
    struct TestCase {
        std::string url;

        std::string protocol;
        std::string host;
        int port;
        std::string path;
        std::string query;
    };

    TestCase cases[] = {
        { "http://www.so.com/query?a=1", "http", "www.so.com", 80, "/query", "a=1" },
        { "http://www.so.com/", "http", "www.so.com", 80, "/", "" },
        { "http://www.so.com", "http", "www.so.com", 80, "", "" },
    };

    for (size_t i = 0; i < H_ARRAYSIZE(cases); i++) {
        evpp::httpc::URLParser p(cases[i].url);
        H_TEST_ASSERT(p.schema == cases[i].protocol);
        H_TEST_ASSERT(p.host == cases[i].host);
        H_TEST_ASSERT(p.port == cases[i].port);
        H_TEST_ASSERT(p.path == cases[i].path);
        H_TEST_ASSERT(p.query == cases[i].query);
    }
}

