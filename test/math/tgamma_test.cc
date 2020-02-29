//
// Created by liyinbin on 2020/2/29.
//

#include "math_test.h"
#include <abel/math/tgamma.h>

TEST(tgamma, all) {

    MATH_TEST_EQ(abel::tgamma,std::tgamma,  1.5L);
    MATH_TEST_EQ(abel::tgamma,std::tgamma,  2.7L);
    MATH_TEST_EQ(abel::tgamma,std::tgamma,  3.0L);
    MATH_TEST_EQ(abel::tgamma,std::tgamma,  4.0L);
    MATH_TEST_EQ(abel::tgamma,std::tgamma,  5.0L);

    MATH_TEST_EQ(abel::tgamma,std::tgamma, TEST_NAN);
    MATH_TEST_EQ(abel::tgamma,std::tgamma, 1.0);

    MATH_TEST_EQ(abel::tgamma,std::tgamma, 0.9);
    MATH_TEST_EQ(abel::tgamma,std::tgamma, 0.1);
    MATH_TEST_EQ(abel::tgamma,std::tgamma, 0.001);
    MATH_TEST_EQ(abel::tgamma,std::tgamma, 0.0);

    MATH_TEST_EQ(abel::tgamma,std::tgamma, -0.1);
    MATH_TEST_EQ(abel::tgamma,std::tgamma, -1);
    MATH_TEST_EQ(abel::tgamma,std::tgamma, -1.1);
    MATH_TEST_EQ(abel::tgamma,std::tgamma, -2);
    MATH_TEST_EQ(abel::tgamma,std::tgamma, -3);
    MATH_TEST_EQ(abel::tgamma,std::tgamma, -4);
}