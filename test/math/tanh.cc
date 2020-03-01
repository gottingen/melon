//
// Created by liyinbin on 2020/2/29.
//

#include "math_test.h"
#include <abel/math/tanh.h>

TEST(tanh, all) {

    MATH_TEST_EQ(abel::tanh,std::tanh,  0.0L);
    MATH_TEST_EQ(abel::tanh,std::tanh,  0.001L);
    MATH_TEST_EQ(abel::tanh,std::tanh,  0.5L);
    MATH_TEST_EQ(abel::tanh,std::tanh, -0.5L);
    MATH_TEST_EQ(abel::tanh,std::tanh,  0.7568025L);
    MATH_TEST_EQ(abel::tanh,std::tanh,  1.0L);
    MATH_TEST_EQ(abel::tanh,std::tanh,  5.0L);

    MATH_TEST_EQ(abel::tanh,std::tanh, TEST_NAN);
}