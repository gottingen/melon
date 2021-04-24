// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com


//
// Created by liyinbin on 2019/10/27.
//

#include "abel/metrics/gauge.h"
#include "gtest/gtest.h"

using abel::metrics::gauge;

TEST(GaugeTest, initialize_with_zero) {
    gauge gau;
    EXPECT_EQ(gau.value(), 0);
}

TEST(GaugeTest, inc) {
    gauge gau;
    gau.inc();
    EXPECT_EQ(gau.value(), 1.0);
}

TEST(GaugeTest, inc_number
) {
    gauge gau;
    gau.inc(4);
    EXPECT_EQ(gau.value(), 4.0);
}

TEST(GaugeTest, inc_multiple) {
    gauge gau;
    gau.inc();
    gau.inc();
    gau.inc(5);
    EXPECT_EQ(gau.value(), 7.0);
}

TEST(GaugeTest, inc_negative_value) {
    gauge gau;
    gau.inc(5.0);
    gau.inc(-5.0);
    EXPECT_EQ(gau.value(), 5.0);
}

TEST(GaugeTest, dec) {
    gauge gau;
    gau.update(5.0);
    gau.dec();
    EXPECT_EQ(gau.value(), 4.0);
}

TEST(GaugeTest, dec_negative_value
) {
    gauge gau;
    gau.update(5.0);
    gau.dec(-1.0);
    EXPECT_EQ(gau.value(), 5.0);
}

TEST(GaugeTest, dec_number
) {
    gauge gau;
    gau.update(5.0);
    gau.dec(3.0);
    EXPECT_EQ(gau.value(), 2.0);
}

TEST(GaugeTest, set) {
    gauge gau;
    gau.update(3.0);
    EXPECT_EQ(gau.value(), 3.0);
}

TEST(GaugeTest, set_multiple) {

    gauge gau;
    gau.update(3.0);
    gau.update(8.0);
    gau.update(1.0);
    EXPECT_EQ(gau.value(), 1.0);
}

