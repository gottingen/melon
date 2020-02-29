//
// Created by liyinbin on 2020/2/29.
//


#include "math_test.h"
#include <abel/math/atan2.h>

TEST(atan2, all) {

    MATH_TEST_EQ(abel::atan2, std::atan2, 0.0L,           0.0L);
    MATH_TEST_EQ(abel::atan2, std::atan2, -0.0L,           0.0L);
    MATH_TEST_EQ(abel::atan2, std::atan2, 0.0L,           -0.0L);
    MATH_TEST_EQ(abel::atan2, std::atan2, -0.0L,           -0.0L);



    MATH_TEST_EQ(abel::atan2,std::atan2,      0.2L,           0.0L);
    MATH_TEST_EQ(abel::atan2,std::atan2,     -0.2L,           0.0L);
    MATH_TEST_EQ(abel::atan2,std::atan2,      0.001L,         0.001L);
    MATH_TEST_EQ(abel::atan2,std::atan2,      0.49L,          0.49L);

    MATH_TEST_EQ(abel::atan2,std::atan2,     -0.5L,          -0.5L);
    MATH_TEST_EQ(abel::atan2,std::atan2,      0.5L,          -0.5L);
    MATH_TEST_EQ(abel::atan2,std::atan2,     -0.5L,           0.5L);

    MATH_TEST_EQ(abel::atan2,std::atan2,      9.6L,           8.4L);
    MATH_TEST_EQ(abel::atan2,std::atan2,      1.0L,           0.0L);
    MATH_TEST_EQ(abel::atan2,std::atan2,      0.0L,           1.0L);
    MATH_TEST_EQ(abel::atan2,std::atan2,     -1.0L,           0.0L);
    MATH_TEST_EQ(abel::atan2,std::atan2,      0.0L,          -1.0L);
    MATH_TEST_EQ(abel::atan2,std::atan2,      1.0L,           3.0L);
    MATH_TEST_EQ(abel::atan2,std::atan2,     -5.0L,           2.5L);
    MATH_TEST_EQ(abel::atan2,std::atan2,  -1000.0L,          -0.001L);
    MATH_TEST_EQ(abel::atan2,std::atan2,      0.1337L,  -123456.0L);

    //

    MATH_TEST_EQ(abel::atan2,std::atan2, std::numeric_limits<double>::quiet_NaN(),     1.0L);
    MATH_TEST_EQ(abel::atan2,std::atan2,     1.0L, std::numeric_limits<double>::quiet_NaN());
    MATH_TEST_EQ(abel::atan2,std::atan2, std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::quiet_NaN());

}
