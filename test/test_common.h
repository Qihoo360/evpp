#pragma once

#include <gtest/gtest.h>

//
//   How to write a unit test:
//   Please see <code>*_test.cc</code>
//
//   Usage:
//
//   TEST_UNIT(sample_test)
//   {
//       int a = 1;
//       int b = a;
//       H_TEST_ASSERT(a == b);
//       int c = a + b;
//       H_TEST_ASSERT(c == a + b);
//
//       H_TEST_ASSERT(printf("Please using H_TEST_ASSERT instead of assert, H_ASSERT, CPPUNIT_ASSERT!\n"));
//       H_TEST_ASSERT(false || printf("If assert failed, we can use printf to print this some error message, because printf return int!\n"));
//   }
//
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



// from google3/base/basictypes.h
// The H_ARRAYSIZE(arr) macro returns the # of elements in an array arr.
// The expression is a compile-time constant, and therefore can be
// used in defining new arrays, for example.
//
// H_ARRAYSIZE catches a few type errors.  If you see a compiler error
//
//   "warning: division by zero in ..."
//
// when using H_ARRAYSIZE, you are (wrongfully) giving it a pointer.
// You should only use H_ARRAYSIZE on statically allocated arrays.
//
// The following comments are on the implementation details, and can
// be ignored by the users.
//
// ARRAYSIZE(arr) works by inspecting sizeof(arr) (the # of bytes in
// the array) and sizeof(*(arr)) (the # of bytes in one array
// element).  If the former is divisible by the latter, perhaps arr is
// indeed an array, in which case the division result is the # of
// elements in the array.  Otherwise, arr cannot possibly be an array,
// and we generate a compiler error to prevent the code from
// compiling.
//
// Since the size of bool is implementation-defined, we need to cast
// !(sizeof(a) & sizeof(*(a))) to size_t in order to ensure the final
// result has type size_t.
//
// This macro is not perfect as it wrongfully accepts certain
// pointers, namely where the pointer size is divisible by the pointer
// size.  Since all our code has to go through a 32-bit compiler,
// where a pointer is 4 bytes, this means all pointers to a type whose
// size is 3 or greater than 4 will be (righteously) rejected.
//
// Kudos to Jorg Brown for this simple and elegant implementation.
#undef H_ARRAYSIZE
#define H_ARRAYSIZE(a) \
    ((sizeof(a) / sizeof(*(a))) / \
     static_cast<size_t>(!(sizeof(a) % sizeof(*(a)))))
