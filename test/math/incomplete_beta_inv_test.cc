//
// Created by liyinbin on 2020/2/29.
//

#include "math_test.h"
#include <abel/math/incomplete_beta_inv.h>

TEST(incomplete_beta_inv, all) {


    MATH_TEST_EXPECT(abel::incomplete_beta_inv, 0.085977260425697907276L,  0.9L,  0.9L,  0.1L);
    MATH_TEST_EXPECT(abel::incomplete_beta_inv, 0.81533908558467627081L,   0.9L,  0.9L,  0.8L);

    MATH_TEST_EXPECT(abel::incomplete_beta_inv, 0.80000000000000004441L,   1.0L,  1.0L,  0.8L);
    MATH_TEST_EXPECT(abel::incomplete_beta_inv, 0.71285927458325959449L,   2.0L,  2.0L,  0.8L);
    MATH_TEST_EXPECT(abel::incomplete_beta_inv, 0.78768287172204876079L,   3.0L,  2.0L,  0.8L);

    MATH_TEST_EXPECT(abel::incomplete_beta_inv, 0.58245357452433332845L,   2.0L,  3.0L,  0.8L);
    MATH_TEST_EXPECT(abel::incomplete_beta_inv, 0.064038139102833388505L,  3.0L,  2.0L,  0.001L);
    MATH_TEST_EXPECT(abel::incomplete_beta_inv, 0.43293107714773171324L,   2.0L,  2.0L,  0.4L);

    //

    MATH_TEST_EXPECT(abel::incomplete_beta_inv, TEST_NAN,  TEST_NAN,      3.0L,      0.8L);
    MATH_TEST_EXPECT(abel::incomplete_beta_inv, TEST_NAN,      3.0L,  TEST_NAN,      0.8L);
    MATH_TEST_EXPECT(abel::incomplete_beta_inv, TEST_NAN,      3.0L,      3.0L,  TEST_NAN);


}