#ifndef GOOGLE_TEST_MICRO_WRAPPER_H_
#define GOOGLE_TEST_MICRO_WRAPPER_H_

#include <gtest/gtest.h>

/**
*    How to write a unit test:
*    Please see <code>*_test.cc</code>
*
*    Usage:
*
*    TEST_UNIT(sample_test)
*    {
*        int a = 1;
*        int b = a;
*        H_TEST_ASSERT(a == b);
*        int c = a + b;
*        H_TEST_ASSERT(c == a + b);
*
*        H_TEST_ASSERT(printf("Please using H_TEST_ASSERT instead of assert, H_ASSERT, CPPUNIT_ASSERT!\n"));
*        H_TEST_ASSERT(false || printf("If assert failed, we can use printf to print this some error message, because printf return int!\n"));
*    }
*/

#define TEST_UNIT(name)  \
class GtestObjectClass_##name : public testing::Test{ \
public: \
    GtestObjectClass_##name() {} \
    ~GtestObjectClass_##name() {} \
    virtual void SetUp() {} \
    virtual void TearDown() {} \
}; \
    TEST_F(GtestObjectClass_##name, name)

#define H_TEST_ASSERT ASSERT_TRUE
#define H_TEST_EQUAL(x, y) H_TEST_ASSERT((x)==(y))
#define H_EXPECT_EQUAL(x, y) H_TEST_ASSERT((x)==(y))
#define H_EXPECT_TRUE(x) H_TEST_ASSERT(x)
#define H_EXPECT_FALSE(x) H_TEST_ASSERT(!(x))

#define H_ARRAYSIZE(a) \
    ((sizeof(a) / sizeof(*(a))) / \
    static_cast<size_t>(!(sizeof(a) % sizeof(*(a)))))


#endif

