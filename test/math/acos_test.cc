//
// Created by liyinbin on 2020/2/29.
//

#include "math_test.h"
#include <abel/math/acos.h>

TEST(acos, all) {

    MATH_TEST_EQ(abel::acos, std::acos, 0.0l);
    MATH_TEST_EQ(abel::acos, std::acos, 0.001L);
    MATH_TEST_EQ(abel::acos, std::acos, 0.5L);
    MATH_TEST_EQ(abel::acos, std::acos, 0.7568025L);
    MATH_TEST_EQ(abel::acos, std::acos, 1.0L);
    MATH_TEST_EQ(abel::acos, std::acos, 5.0L);
    MATH_TEST_EQ(abel::acos, std::acos, std::numeric_limits<double>::quiet_NaN());
}


