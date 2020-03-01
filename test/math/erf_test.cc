//
// Created by liyinbin on 2020/2/29.
//


#include "math_test.h"
#include <abel/math/erf.h>

TEST(erf, all) {

    MATH_TEST_EQ(abel::erf, std::erf, -3.0L);
    MATH_TEST_EQ(abel::erf, std::erf, -2.5L);
    MATH_TEST_EQ(abel::erf, std::erf, -2.11L);
    MATH_TEST_EQ(abel::erf, std::erf, -2.05L);
    MATH_TEST_EQ(abel::erf, std::erf, -1.3L);
    MATH_TEST_EQ(abel::erf, std::erf,  0.0L);
    MATH_TEST_EQ(abel::erf, std::erf,  1.3L);
    MATH_TEST_EQ(abel::erf, std::erf,  2.05L);
    MATH_TEST_EQ(abel::erf, std::erf,  2.11L);
    MATH_TEST_EQ(abel::erf, std::erf,  2.5L);
    MATH_TEST_EQ(abel::erf, std::erf,  3.0L);

    //

    MATH_TEST_EQ(abel::erf, std::erf, TEST_NAN);
    MATH_TEST_EQ(abel::erf, std::erf, TEST_POSINF);
    MATH_TEST_EQ(abel::erf, std::erf, TEST_NEGINF);


}