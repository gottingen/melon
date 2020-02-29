//
// Created by liyinbin on 2020/2/29.
//


#include "math_test.h"
#include <abel/math/sinh.h>

TEST(sinh, all) {

    MATH_TEST_EQ(abel::sinh,std::sinh,0.0L);
    MATH_TEST_EQ(abel::sinh,std::sinh,0.001L);
    MATH_TEST_EQ(abel::sinh,std::sinh,0.5L);
    MATH_TEST_EQ(abel::sinh,std::sinh,-0.5L);
    MATH_TEST_EQ(abel::sinh,std::sinh,0.7568025L);
    MATH_TEST_EQ(abel::sinh,std::sinh,1.0L);
    MATH_TEST_EQ(abel::sinh,std::sinh,5.0L);

    MATH_TEST_EQ(abel::sinh,std::sinh,TEST_NAN);
}