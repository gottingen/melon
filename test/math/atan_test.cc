//
// Created by liyinbin on 2020/2/29.
//

#include "math_test.h"
#include <abel/math/atan.h>

TEST(atan, all) {

    MATH_TEST_EQ(abel::atan, std::atan, 0.0l);
    MATH_TEST_EQ(abel::atan, std::atan, 0.001L);
    MATH_TEST_EQ(abel::atan, std::atan, 0.49L);
    MATH_TEST_EQ(abel::atan, std::atan, -0.5L);
    MATH_TEST_EQ(abel::atan, std::atan, -1.5L);
    MATH_TEST_EQ(abel::atan, std::atan, 0.7568025L);
    MATH_TEST_EQ(abel::atan, std::atan, 0.99L);
    MATH_TEST_EQ(abel::atan, std::atan, 1.49L);
    MATH_TEST_EQ(abel::atan, std::atan, 1.99L);

    MATH_TEST_EQ(abel::atan, std::atan, 2.49);
    MATH_TEST_EQ(abel::atan, std::atan, 2.51L);
    MATH_TEST_EQ(abel::atan, std::atan, 3.99L);
    MATH_TEST_EQ(abel::atan, std::atan, 7.0L);
    MATH_TEST_EQ(abel::atan, std::atan, 11.0L);
    MATH_TEST_EQ(abel::atan, std::atan, 25.0L);
    MATH_TEST_EQ(abel::atan, std::atan, 101.0L);
    MATH_TEST_EQ(abel::atan, std::atan, 900L);
    MATH_TEST_EQ(abel::atan, std::atan, 1001.0L);
    MATH_TEST_EQ(abel::atan, std::atan, std::numeric_limits<double>::quiet_NaN());
}
