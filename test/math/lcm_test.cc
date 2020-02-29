//
// Created by liyinbin on 2020/2/29.
//

#include "math_test.h"
#include <abel/math/lcm.h>

TEST(lcm, all) {


    MATH_TEST_EXPECT(abel::lcm,12,3,4);
    MATH_TEST_EXPECT(abel::lcm,60,12,15);
    MATH_TEST_EXPECT(abel::lcm,1377,17,81);


}