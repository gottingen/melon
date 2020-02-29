//
// Created by liyinbin on 2020/2/29.
//


#include "math_test.h"
#include <abel/math/sqrt.h>

TEST(sqrt, all) {

    MATH_TEST_EQ(abel::sqrt,std::sqrt,  0.5L);
    MATH_TEST_EQ(abel::sqrt,std::sqrt,  0.00199900000000000208L);
    MATH_TEST_EQ(abel::sqrt,std::sqrt,  1.5L);
    MATH_TEST_EQ(abel::sqrt,std::sqrt,  2.0L);
    MATH_TEST_EQ(abel::sqrt,std::sqrt,  41.5L);
    MATH_TEST_EQ(abel::sqrt,std::sqrt,  0.0L);
    MATH_TEST_EQ(abel::sqrt,std::sqrt, -1.0L);

    MATH_TEST_EQ(abel::sqrt,std::sqrt, -std::numeric_limits<long double>::infinity());
    MATH_TEST_EQ(abel::sqrt,std::sqrt,  std::numeric_limits<long double>::infinity());
    MATH_TEST_EQ(abel::sqrt,std::sqrt,  std::numeric_limits<long double>::quiet_NaN());
}