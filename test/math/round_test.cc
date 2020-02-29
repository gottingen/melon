//
// Created by liyinbin on 2020/2/29.
//


#include "math_test.h"
#include <abel/math/round.h>
#include <abel/math/floor.h>
#include <abel/math/ceil.h>
#include <abel/math/trunc.h>

TEST(pow, all) {

    MATH_TEST_EQ(abel::floor,std::floor,  0.0);
    MATH_TEST_EQ(abel::floor,std::floor, -0.0);
    MATH_TEST_EQ(abel::floor,std::floor,  4.2);
    MATH_TEST_EQ(abel::floor,std::floor,  4.5);
    MATH_TEST_EQ(abel::floor,std::floor,  4.7);
    MATH_TEST_EQ(abel::floor,std::floor,  5.0);
    MATH_TEST_EQ(abel::floor,std::floor, -4.2);
    MATH_TEST_EQ(abel::floor,std::floor, -4.7);
    MATH_TEST_EQ(abel::floor,std::floor, -5.0);

    MATH_TEST_EQ(abel::floor,std::floor, -std::numeric_limits<long double>::infinity());
    MATH_TEST_EQ(abel::floor,std::floor,  std::numeric_limits<long double>::infinity());
    MATH_TEST_EQ(abel::floor,std::floor,  std::numeric_limits<long double>::quiet_NaN());

    //

    MATH_TEST_EQ(abel::ceil,std::ceil,  0.0);
    MATH_TEST_EQ(abel::ceil,std::ceil, -0.0);
    MATH_TEST_EQ(abel::ceil,std::ceil,  4.2);
    MATH_TEST_EQ(abel::ceil,std::ceil,  4.5);
    MATH_TEST_EQ(abel::ceil,std::ceil,  4.7);
    MATH_TEST_EQ(abel::ceil,std::ceil,  5.0);
    MATH_TEST_EQ(abel::ceil,std::ceil, -4.2);
    MATH_TEST_EQ(abel::ceil,std::ceil, -4.7);
    MATH_TEST_EQ(abel::ceil,std::ceil, -5.0);

    MATH_TEST_EQ(abel::ceil,std::ceil, -std::numeric_limits<long double>::infinity());
    MATH_TEST_EQ(abel::ceil,std::ceil,  std::numeric_limits<long double>::infinity());
    MATH_TEST_EQ(abel::ceil,std::ceil,  std::numeric_limits<long double>::quiet_NaN());

    //

    MATH_TEST_EQ(abel::trunc,std::trunc,  0.0);
    MATH_TEST_EQ(abel::trunc,std::trunc, -0.0);
    MATH_TEST_EQ(abel::trunc,std::trunc,  4.2);
    MATH_TEST_EQ(abel::trunc,std::trunc,  4.5);
    MATH_TEST_EQ(abel::trunc,std::trunc,  4.7);
    MATH_TEST_EQ(abel::trunc,std::trunc,  5.0);
    MATH_TEST_EQ(abel::trunc,std::trunc, -4.2);
    MATH_TEST_EQ(abel::trunc,std::trunc, -4.7);
    MATH_TEST_EQ(abel::trunc,std::trunc, -5.0);

    MATH_TEST_EQ(abel::trunc,std::trunc, -std::numeric_limits<long double>::infinity());
    MATH_TEST_EQ(abel::trunc,std::trunc,  std::numeric_limits<long double>::infinity());
    MATH_TEST_EQ(abel::trunc,std::trunc,  std::numeric_limits<long double>::quiet_NaN());

    //

    MATH_TEST_EQ(abel::round,std::round,  0.0);
    MATH_TEST_EQ(abel::round,std::round, -0.0);
    MATH_TEST_EQ(abel::round,std::round,  4.2);
    MATH_TEST_EQ(abel::round,std::round,  4.5);
    MATH_TEST_EQ(abel::round,std::round,  4.7);
    MATH_TEST_EQ(abel::round,std::round,  5.0);
    MATH_TEST_EQ(abel::round,std::round, -4.2);
    MATH_TEST_EQ(abel::round,std::round, -4.5);
    MATH_TEST_EQ(abel::round,std::round, -4.7);
    MATH_TEST_EQ(abel::round,std::round, -5.0);

    MATH_TEST_EQ(abel::round,std::round, -std::numeric_limits<long double>::infinity());
    MATH_TEST_EQ(abel::round,std::round,  std::numeric_limits<long double>::infinity());
    MATH_TEST_EQ(abel::round,std::round,  std::numeric_limits<long double>::quiet_NaN());
}