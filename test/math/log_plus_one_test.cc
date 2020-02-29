//
// Created by liyinbin on 2020/2/29.
//

#include "math_test.h"
#include <abel/math/log_plus_one.h>

TEST(log, all) {

    MATH_TEST_EQ(abel::log1p,std::log1p,1.0L);
    MATH_TEST_EQ(abel::log1p,std::log1p,0.0L);
    MATH_TEST_EQ(abel::log1p,std::log1p,1e-04L);
    MATH_TEST_EQ(abel::log1p,std::log1p,-1e-04L);
    MATH_TEST_EQ(abel::log1p,std::log1p,1e-05L);
    MATH_TEST_EQ(abel::log1p,std::log1p,1e-06L);
    MATH_TEST_EQ(abel::log1p,std::log1p,1e-22L);

    MATH_TEST_EQ(abel::log1p,std::log1p, -std::numeric_limits<long double>::infinity());
    MATH_TEST_EQ(abel::log1p,std::log1p,  std::numeric_limits<long double>::infinity());
    MATH_TEST_EQ(abel::log1p,std::log1p,  std::numeric_limits<long double>::quiet_NaN());

}