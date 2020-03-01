//
// Created by liyinbin on 2020/2/29.
//

#include "math_test.h"
#include <abel/math/pow.h>

TEST(pow, all) {
    
    MATH_TEST_EQ(abel::pow,std::pow, 0.199900000000000208L, 3.5L);
    MATH_TEST_EQ(abel::pow,std::pow, 0.5L,  2.0L);
    MATH_TEST_EQ(abel::pow,std::pow, 1.5L,  0.99L);
    MATH_TEST_EQ(abel::pow,std::pow, 41.5L, 7.0L);

    // int versions
    MATH_TEST_EQ(abel::pow,std::pow, 0.5L,  2L);
    MATH_TEST_EQ(abel::pow,std::pow, 41.5L, 7L);

    MATH_TEST_EQ(abel::pow,std::pow,  std::numeric_limits<long double>::quiet_NaN(), 2);
    MATH_TEST_EQ(abel::pow,std::pow,  2, std::numeric_limits<long double>::quiet_NaN());

    MATH_TEST_EQ(abel::pow,std::pow, std::numeric_limits<long double>::infinity(), 2);
    MATH_TEST_EQ(abel::pow,std::pow, std::numeric_limits<long double>::infinity(), - 2);
    MATH_TEST_EQ(abel::pow,std::pow, std::numeric_limits<long double>::infinity(), 0);
    
}