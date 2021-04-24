// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com


//
// Created by liyinbin on 2019/10/27.
//

#include "abel/metrics/counter.h"
#include "gtest/gtest.h"

using abel::metrics::counter;

TEST(CounterTest, initialize_with_zero) {
    counter ctr;
    EXPECT_EQ(ctr.value(), 0);
}

TEST(CounterTest, inc) {
    counter ctr;
    ctr.inc();
    EXPECT_EQ(ctr.value(), 1.0);
}

TEST(CounterTest, inc_number) {
    counter ctr;
    ctr.inc(4);
    EXPECT_EQ(ctr.value(), 4.0);
}

TEST(CounterTest, inc_multiple) {
    counter ctr;
    ctr.inc();
    ctr.inc();
    ctr.inc(5);
    EXPECT_EQ(ctr.value(), 7.0);
}

TEST(CounterTest, inc_negative_value
) {
    counter ctr;
    ctr.inc(5.0);
    ctr.inc(-5.0);
    EXPECT_EQ(ctr.value(), 5.0);
}


