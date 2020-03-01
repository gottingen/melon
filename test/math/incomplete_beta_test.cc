//
// Created by liyinbin on 2020/2/29.
//


#include "math_test.h"
#include <abel/math/incomplete_beta.h>

TEST(incomplete_beta, all) {


    MATH_TEST_EXPECT(abel::incomplete_beta, 0.11464699677582491921L,     0.9L,  0.9L,  0.1L);
    MATH_TEST_EXPECT(abel::incomplete_beta, 0.78492840804657726395L,     0.9L,  0.9L,  0.8L);

    MATH_TEST_EXPECT(abel::incomplete_beta, 0.80000000000000004441L,     1.0L,  1.0L,  0.8L);
    MATH_TEST_EXPECT(abel::incomplete_beta, 0.89600000000000001865L,     2.0L,  2.0L,  0.8L);
    MATH_TEST_EXPECT(abel::incomplete_beta, 0.81920000000000003926L,     3.0L,  2.0L,  0.8L);

    MATH_TEST_EXPECT(abel::incomplete_beta, 0.97279999999999999805L,     2.0L,  3.0L,  0.8L);
    MATH_TEST_EXPECT(abel::incomplete_beta, 3.9970000000000084279e-09L,  3.0L,  2.0L,  0.001L);
    MATH_TEST_EXPECT(abel::incomplete_beta, 0.35200000000000003508L,     2.0L,  2.0L,  0.4L);

    MATH_TEST_EXPECT(abel::incomplete_beta, TEST_NAN, TEST_NAN, TEST_NAN, TEST_NAN);
    MATH_TEST_EXPECT(abel::incomplete_beta, TEST_NAN, TEST_NAN,     2.0L,     0.8L);
    MATH_TEST_EXPECT(abel::incomplete_beta, TEST_NAN,     2.0L, TEST_NAN,     0.8L);
    MATH_TEST_EXPECT(abel::incomplete_beta, TEST_NAN,     2.0L,     2.0L, TEST_NAN);



}