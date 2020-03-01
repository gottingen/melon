//
// Created by liyinbin on 2020/2/29.
//



#include "math_test.h"
#include <abel/math/exp.h>

TEST(exp, all) {

    MATH_TEST_EQ(abel::exp,std::exp, -40.0L);
    MATH_TEST_EQ(abel::exp,std::exp, -4.0L);
    MATH_TEST_EQ(abel::exp,std::exp,  0.0001L);
    MATH_TEST_EQ(abel::exp,std::exp,  1.75L);
    MATH_TEST_EQ(abel::exp,std::exp,  1.9991L);
    MATH_TEST_EQ(abel::exp,std::exp,  2.1L);
    MATH_TEST_EQ(abel::exp,std::exp,  4.0L);

    MATH_TEST_EQ(abel::exp,std::exp, -std::numeric_limits<long double>::infinity());
    MATH_TEST_EQ(abel::exp,std::exp,  std::numeric_limits<long double>::infinity());
    MATH_TEST_EQ(abel::exp,std::exp,  std::numeric_limits<long double>::quiet_NaN());


}