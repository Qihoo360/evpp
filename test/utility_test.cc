#include "test_common.h"

#include <evpp/exp.h>
#include <evpp/utility.h>

TEST_UNIT(testCast) {
    int i = 1;
    H_TEST_ASSERT(evpp::cast<int>(i) == "1");
}

TEST_UNIT(testStringSplit1) {
    std::string s = "a,b,c,d";
    std::vector<std::string> v;
    evpp::StringSplit(s, ',', 0, v);
    H_TEST_ASSERT(v.size() == 4);
    H_TEST_ASSERT(v[0] == "a");
    H_TEST_ASSERT(v[1] == "b");
    H_TEST_ASSERT(v[2] == "c");
    H_TEST_ASSERT(v[3] == "d");
}

TEST_UNIT(testStringSplit2) {
    std::string s = "a,b,c,d";
    std::vector<std::string> v;
    evpp::StringSplit(s, ",", 0, v);
    H_TEST_ASSERT(v.size() == 4);
    H_TEST_ASSERT(v[0] == "a");
    H_TEST_ASSERT(v[1] == "b");
    H_TEST_ASSERT(v[2] == "c");
    H_TEST_ASSERT(v[3] == "d");
}
