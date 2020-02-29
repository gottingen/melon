//
// Created by liyinbin on 2020/2/29.
//

#include "math_test.h"
#include <abel/math/acosh.h>

TEST(acosh, all) {

    MATH_TEST_EQ(abel::acosh, std::acosh, 0.0l);
    MATH_TEST_EQ(abel::acosh, std::acosh, 0.001L);
    MATH_TEST_EQ(abel::acosh, std::acosh, 0.5L);
    MATH_TEST_EQ(abel::acosh, std::acosh, 0.7568025L);
    MATH_TEST_EQ(abel::acosh, std::acosh, 1.0L);
    MATH_TEST_EQ(abel::acosh, std::acosh, 5.0L);
    MATH_TEST_EQ(abel::acosh, std::acosh, std::numeric_limits<double>::quiet_NaN());
}

