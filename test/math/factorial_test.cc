//
// Created by liyinbin on 2020/2/29.
//

#include "math_test.h"
#include <abel/math/factorial.h>

TEST(factorial, all) {

    std::function<long double (long double)> std_fn  = [] (long double x) -> long double { return std::tgamma(x+1); };

    //

    // using long doubles -> tgamma
    MATH_TEST_EQ(abel::factorial,std_fn, 3.1L);
    MATH_TEST_EQ(abel::factorial,std_fn, 7.0L);
    MATH_TEST_EQ(abel::factorial,std_fn, 10.0L);
    MATH_TEST_EQ(abel::factorial,std_fn, 12.0L);
    MATH_TEST_EQ(abel::factorial,std_fn, 14.0L);
    MATH_TEST_EQ(abel::factorial,std_fn, 15.0L);

    // using long ints
    MATH_TEST_EQ(abel::factorial,std_fn, 5L);
    MATH_TEST_EQ(abel::factorial,std_fn, 9L);
    MATH_TEST_EQ(abel::factorial,std_fn, 11L);
#ifndef _WIN32
    MATH_TEST_EQ(abel::factorial,std_fn, 13L);
    MATH_TEST_EQ(abel::factorial,std_fn, 16L);
#endif



}