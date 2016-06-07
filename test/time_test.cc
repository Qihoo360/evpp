
#include "evpp/exp.h"
#include "./test_common.h"
#include "evpp/base/duration.h"
#include "evpp/base/timestamp.h"

TEST_UNIT(testDuration)
{
    evpp::base::Duration d0(0);
    evpp::base::Duration d1(1);
    evpp::base::Duration d2(2);
    evpp::base::Duration d3(2);
    H_TEST_ASSERT(d0 < d1);
    H_TEST_ASSERT(d1 < d2);
    H_TEST_ASSERT(d2 == d3);
    H_TEST_ASSERT(d0.IsZero());
}

TEST_UNIT(testTimestamp)
{
}

