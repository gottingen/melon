//
// Created by liyinbin on 2020/2/29.
//

#include "math_test.h"
#include <abel/math/log_gamma.h>

TEST(lgamma, all) {

    MATH_TEST_EQ(abel::lgamma,std::lgamma,  1.5L);
    MATH_TEST_EQ(abel::lgamma,std::lgamma,  0.7L);
    MATH_TEST_EQ(abel::lgamma,std::lgamma,  1.0L);
    MATH_TEST_EQ(abel::lgamma,std::lgamma,  0.0L);
    MATH_TEST_EQ(abel::lgamma,std::lgamma, -1.0L);

    MATH_TEST_EQ(abel::lgamma,std::lgamma, TEST_NAN);


}