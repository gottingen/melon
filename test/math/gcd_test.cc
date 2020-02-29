//
// Created by liyinbin on 2020/2/29.
//

#include "math_test.h"
#include <abel/math/gcd.h>

TEST(gcd, all) {

    MATH_TEST_EXPECT(abel::gcd,6,12,18);
    MATH_TEST_EXPECT(abel::gcd,2,-4,14);
    MATH_TEST_EXPECT(abel::gcd,14,42,56);

}