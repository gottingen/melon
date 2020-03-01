//
// Created by liyinbin on 2020/2/29.
//



#include "math_test.h"
#include <abel/math/erf_inv.h>

TEST(erf_inv, all) {

    MATH_TEST_EXPECT(abel::erf_inv, -3.0L,  -0.999977909503001415L);
    MATH_TEST_EXPECT(abel::erf_inv, -2.5L,  -0.999593047982555041L);
    MATH_TEST_EXPECT(abel::erf_inv, -2.11L, -0.997154845031177802L);
    MATH_TEST_EXPECT(abel::erf_inv, -2.05L, -0.996258096044456873L);
    MATH_TEST_EXPECT(abel::erf_inv, -1.3L,  -0.934007944940652437L);
    MATH_TEST_EXPECT(abel::erf_inv,  0.0L,   0.0L);
    MATH_TEST_EXPECT(abel::erf_inv,  1.3L,   0.934007944940652437L);
    MATH_TEST_EXPECT(abel::erf_inv,  2.05L,  0.996258096044456873L);
    MATH_TEST_EXPECT(abel::erf_inv,  2.11L,  0.997154845031177802L);
    MATH_TEST_EXPECT(abel::erf_inv,  2.5L,   0.999593047982555041L);
    MATH_TEST_EXPECT(abel::erf_inv,  3.0L,   0.999977909503001415L);

    //

    MATH_TEST_EXPECT(abel::erf_inv, TEST_NAN, TEST_NAN);
    MATH_TEST_EXPECT(abel::erf_inv, TEST_NAN, -1.1);
    MATH_TEST_EXPECT(abel::erf_inv, TEST_NAN,  1.1);

    MATH_TEST_EXPECT(abel::erf_inv, TEST_POSINF, 1);
    MATH_TEST_EXPECT(abel::erf_inv, TEST_NEGINF, -1);

}