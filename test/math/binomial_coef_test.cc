//
// Created by liyinbin on 2020/2/29.
//


#include "math_test.h"
#include <abel/math/binomial_coef.h>

TEST(binomial_coef, all) {

    MATH_TEST_EXPECT(abel::binomial_coef,  1,  0,  0);
    MATH_TEST_EXPECT(abel::binomial_coef,  0,  0,  1);
    MATH_TEST_EXPECT(abel::binomial_coef,  1,  1,  0);
    MATH_TEST_EXPECT(abel::binomial_coef,  1,  1,  1);
    MATH_TEST_EXPECT(abel::binomial_coef, 10,  5,  2);
    MATH_TEST_EXPECT(abel::binomial_coef, 45, 10,  8);
    MATH_TEST_EXPECT(abel::binomial_coef, 10, 10,  9);
    MATH_TEST_EXPECT(abel::binomial_coef,  1, 10, 10);
}