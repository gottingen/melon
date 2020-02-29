//
// Created by liyinbin on 2020/2/29.
//


#include "math_test.h"
#include <abel/math/sin.h>

TEST(sin, all) {

    MATH_TEST_EQ(abel::sin,std::sin,-1.5L);
    MATH_TEST_EQ(abel::sin,std::sin,0.0L);
    MATH_TEST_EQ(abel::sin,std::sin,0.001L);
    MATH_TEST_EQ(abel::sin,std::sin,1.001L);
    MATH_TEST_EQ(abel::sin,std::sin,1.5L);
    MATH_TEST_EQ(abel::sin,std::sin,11.1L);
    MATH_TEST_EQ(abel::sin,std::sin,50.0L);
    MATH_TEST_EQ(abel::sin,std::sin,150.0L);

    MATH_TEST_EQ(abel::sin,std::sin,TEST_NAN);

}