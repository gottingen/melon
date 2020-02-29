//
// Created by liyinbin on 2020/2/29.
//

#include "math_test.h"
#include <abel/math/tan.h>

TEST(tan, all) {

    MATH_TEST_EQ(abel::tan,std::tan, 0.0L);
    MATH_TEST_EQ(abel::tan,std::tan, 0.001L);
    MATH_TEST_EQ(abel::tan,std::tan, 1.001L);
    MATH_TEST_EQ(abel::tan,std::tan, 1.5L);
    MATH_TEST_EQ(abel::tan,std::tan, 11.1L);
    MATH_TEST_EQ(abel::tan,std::tan, 50.0L);
    MATH_TEST_EQ(abel::tan,std::tan, -1.5L);
    // MATH_TEST_EQ(abel::tan,std::tan, lrgval);

    MATH_TEST_EQ(abel::tan,std::tan, TEST_NAN);
}