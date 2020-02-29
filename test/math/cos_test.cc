//
// Created by liyinbin on 2020/2/29.
//

#include "math_test.h"
#include <abel/math/cos.h>

TEST(acos, all) {

    MATH_TEST_EQ(abel::cos,std::cos,-1.5L);
    MATH_TEST_EQ(abel::cos,std::cos,0.0L);
    MATH_TEST_EQ(abel::cos,std::cos,0.001L);
    MATH_TEST_EQ(abel::cos,std::cos,1.001L);
    MATH_TEST_EQ(abel::cos,std::cos,1.5L);
    MATH_TEST_EQ(abel::cos,std::cos,11.1L);
    MATH_TEST_EQ(abel::cos,std::cos,50.0L);
    MATH_TEST_EQ(abel::cos,std::cos,150.0L);

    MATH_TEST_EQ(abel::cos,std::cos,TEST_NAN);
}