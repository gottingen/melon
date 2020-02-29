//
// Created by liyinbin on 2020/2/29.
//

#include "math_test.h"
#include <abel/math/log.h>

TEST(log, all) {

    MATH_TEST_EQ(abel::log,std::log,  0.5L);
    MATH_TEST_EQ(abel::log,std::log,  0.00199900000000000208L);
    MATH_TEST_EQ(abel::log,std::log,  1.5L);
    MATH_TEST_EQ(abel::log,std::log,  41.5L);
    MATH_TEST_EQ(abel::log,std::log,  0.0L);
    MATH_TEST_EQ(abel::log,std::log, -1.0L);

    MATH_TEST_EQ(abel::log,std::log, -std::numeric_limits<long double>::infinity());
    MATH_TEST_EQ(abel::log,std::log,  std::numeric_limits<long double>::infinity());
    MATH_TEST_EQ(abel::log,std::log,  std::numeric_limits<long double>::quiet_NaN());



}