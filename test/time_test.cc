
#include "evpp/exp.h"
#include "./test_common.h"
#include "evpp/duration.h"
#include "evpp/timestamp.h"

TEST_UNIT(testDuration)
{
    evpp::Duration d0(0);
    evpp::Duration d1(1);
    evpp::Duration d2(2);
    evpp::Duration d3(2);
    H_TEST_ASSERT(d0 < d1);
    H_TEST_ASSERT(d1 < d2);
    H_TEST_ASSERT(d2 == d3);
    H_TEST_ASSERT(d0.IsZero());
}

TEST_UNIT(testTimestamp)
{
}

