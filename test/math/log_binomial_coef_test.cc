//
// Created by liyinbin on 2020/2/29.
//

#include "math_test.h"
#include <abel/math/log_binomial_coef.h>

TEST(log_binomial_coef, all) {

    MATH_TEST_EXPECT(abel::log_binomial_coef, 0, 0, 0);
    MATH_TEST_EXPECT(abel::log_binomial_coef, TEST_NEGINF, 0, 1);
    MATH_TEST_EXPECT(abel::log_binomial_coef, 0, 1, 0);
    MATH_TEST_EXPECT(abel::log_binomial_coef, 0, 1, 1);
    MATH_TEST_EXPECT(abel::log_binomial_coef, std::log(10.0L), 5, 2);
    MATH_TEST_EXPECT(abel::log_binomial_coef, std::log(45.0L), 10, 8);
    MATH_TEST_EXPECT(abel::log_binomial_coef, std::log(10.0L), 10, 9);
    MATH_TEST_EXPECT(abel::log_binomial_coef, 0, 10, 10);


}