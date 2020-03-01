//
// Created by liyinbin on 2020/2/29.
//


#include "math_test.h"
#include <abel/math/atanh.h>

TEST(atanh, all) {

    MATH_TEST_EQ(abel::atanh,std::atanh,-0.99L);
    MATH_TEST_EQ(abel::atanh,std::atanh,0.0L);
    MATH_TEST_EQ(abel::atanh,std::atanh,0.001L);
    MATH_TEST_EQ(abel::atanh,std::atanh,1.0L);
    MATH_TEST_EQ(abel::atanh,std::atanh,-1.0L);
    MATH_TEST_EQ(abel::atanh,std::atanh,1.1L);

    MATH_TEST_EQ(abel::atanh,std::atanh,TEST_NAN);

}

