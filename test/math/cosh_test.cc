//
// Created by liyinbin on 2020/2/29.
//



#include "math_test.h"
#include <abel/math/cosh.h>

TEST(acos, all) {

    MATH_TEST_EQ(abel::cosh,std::cosh,0.0L);
    MATH_TEST_EQ(abel::cosh,std::cosh,0.001L);
    MATH_TEST_EQ(abel::cosh,std::cosh,0.5L);
    MATH_TEST_EQ(abel::cosh,std::cosh,-0.5L);
    MATH_TEST_EQ(abel::cosh,std::cosh,0.7568025L);
    MATH_TEST_EQ(abel::cosh,std::cosh,1.0L);
    MATH_TEST_EQ(abel::cosh,std::cosh,5.0L);
    MATH_TEST_EQ(abel::cosh,std::cosh,TEST_NAN);

    MATH_TEST_EQ(abel::cosh,std::cosh,TEST_NAN);
}