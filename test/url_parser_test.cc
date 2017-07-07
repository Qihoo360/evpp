#include "test_common.h"

#include <evpp/httpc/url_parser.h>
#include <evpp/http/context.h>

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

TEST_UNIT(TestFindQueryParamFromURI) {
    struct TestCase {
        std::string uri;
        std::string key;
        std::string value;
    };

    TestCase cases[] = {
        { "/query", "a", ""},
        { "/query?", "abc", ""},
        { "/query?a=1&bc=123", "abc", ""},
        { "/query?a=123", "abc", ""},
        { "/query?a=1", "a", "1"},
        { "/query?a=1&b=2&c=3", "a", "1"},
        { "/query?d=4&a=1&b=2&c=3", "a", "1"},
        { "/query?d=4&b=2&c=3&a=1123123", "a", "1123123"},
        { "/query?ab=1", "ab", "1"},
        { "/query?ab=1&b=2&c=3", "ab", "1"},
        { "/query?d=4&ab=1&b=2&c=3", "ab", "1"},
        { "/query?d=4&b=2&c=3&ab=1123123", "ab", "1123123"},
        { "/query?thekey=1", "thekey", "1"},
        { "/query?thekey=1&b=2&c=3", "thekey", "1"},
        { "/query?d=4&thekey=1&b=2&c=3", "thekey", "1"},
        { "/query?d=4&b=2&c=3&thekey=1123123", "thekey", "1123123"},
    };

    for (size_t i = 0; i < H_ARRAYSIZE(cases); i++) {
        std::string uri = cases[i].uri;
        std::string k = cases[i].key;
        std::string expected_value = cases[i].value;
        std::string v = evpp::http::Context::FindQueryParamFromURI(uri, k);
        H_TEST_ASSERT(v == expected_value);
    }
}