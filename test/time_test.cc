
#include "./test_common.h"
#include "evpp/duration.h"
#include "evpp/timestamp.h"
#include "evpp/gettimeofday.h"

TEST_UNIT(testDuration) {
    evpp::Duration d0(0);
    evpp::Duration d1(1);
    evpp::Duration d2(2);
    evpp::Duration d3(2);
    H_TEST_ASSERT(d0 < d1);
    H_TEST_ASSERT(d1 < d2);
    H_TEST_ASSERT(d2 == d3);
    H_TEST_ASSERT(d0.IsZero());
    H_TEST_ASSERT(d0 <= d1);
    H_TEST_ASSERT(d1 <= d2);
    H_TEST_ASSERT(d2 <= d3);
    H_TEST_ASSERT(d2 >= d3);
    H_TEST_ASSERT(d1 > d0);
    H_TEST_ASSERT(d2 > d1);
    H_TEST_ASSERT(d1 >= d0);
    H_TEST_ASSERT(d2 >= d1);
}

TEST_UNIT(testTimestamp) {
    int64_t c_s = time(nullptr);
    int64_t c_us = evpp::utcmicrosecond();
    int64_t ts_ns = evpp::Timestamp::Now().UnixNano();
    int64_t c11_us = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    H_TEST_ASSERT(c_us / 1000000 == c11_us / 1000000);
    H_TEST_ASSERT(c_s == c11_us / 1000000);
    H_TEST_ASSERT(c_s == ts_ns / evpp::Duration::kSecond);
}

