// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com

#include "math_test.h"
#include "abel/math/is_odd.h"
#include "abel/math/is_even.h"

TEST(is_odd, all) {


    int run_val = 0;
    run_val += abel::is_odd(1);
    run_val += abel::is_odd(3);
    run_val += abel::is_odd(-5);
    run_val += abel::is_even(10UL);
    run_val += abel::is_odd(-400009L);
    run_val += abel::is_even(100000000L);
    EXPECT_EQ(run_val, 6);

}
