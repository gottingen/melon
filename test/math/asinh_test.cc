//
// Created by liyinbin on 2020/2/29.
//


#include "math_test.h"
#include <abel/math/asinh.h>

TEST(asinh, all) {

    MATH_TEST_EQ(abel::asinh, std::asinh, 0.0l);
    MATH_TEST_EQ(abel::asinh, std::asinh, 0.001L);
    MATH_TEST_EQ(abel::asinh, std::asinh, 0.5L);
    MATH_TEST_EQ(abel::asinh, std::asinh, 0.7568025L);
    MATH_TEST_EQ(abel::asinh, std::asinh, 1.0L);
    MATH_TEST_EQ(abel::asinh, std::asinh, 5.0L);
    MATH_TEST_EQ(abel::asinh, std::asinh, std::numeric_limits<double>::quiet_NaN());
}