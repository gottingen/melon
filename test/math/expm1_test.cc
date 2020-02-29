//
// Created by liyinbin on 2020/2/29.
//

#include "math_test.h"
#include <abel/math/expm1.h>

TEST(expm1, all) {

    MATH_TEST_EQ(abel::expm1,std::expm1,1.0L);
    MATH_TEST_EQ(abel::expm1,std::expm1,0.0L);
    MATH_TEST_EQ(abel::expm1,std::expm1,1e-04L);
    MATH_TEST_EQ(abel::expm1,std::expm1,-1e-04L);
    MATH_TEST_EQ(abel::expm1,std::expm1,1e-05L);
    MATH_TEST_EQ(abel::expm1,std::expm1,1e-06L);
    MATH_TEST_EQ(abel::expm1,std::expm1,1e-22L);

    MATH_TEST_EQ(abel::expm1,std::expm1, -std::numeric_limits<long double>::infinity());
    MATH_TEST_EQ(abel::expm1,std::expm1,  std::numeric_limits<long double>::infinity());
    MATH_TEST_EQ(abel::expm1,std::expm1,  std::numeric_limits<long double>::quiet_NaN());


}