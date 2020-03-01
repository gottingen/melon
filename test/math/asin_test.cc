//
// Created by liyinbin on 2020/2/29.
//

#include "math_test.h"
#include <abel/math/asin.h>

TEST(asin, all) {

    MATH_TEST_EQ(abel::asin, std::asin, 0.0l);
    MATH_TEST_EQ(abel::asin, std::asin, 0.001L);
    MATH_TEST_EQ(abel::asin, std::asin, 0.5L);
    MATH_TEST_EQ(abel::asin, std::asin, 0.7568025L);
    MATH_TEST_EQ(abel::asin, std::asin, 1.0L);
    MATH_TEST_EQ(abel::asin, std::asin, 5.0L);
    MATH_TEST_EQ(abel::asin, std::asin, std::numeric_limits<double>::quiet_NaN());
}