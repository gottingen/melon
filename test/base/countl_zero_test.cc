// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com

#include "abel/base/math/countl_zero.h"
#include "gtest/gtest.h"

int CLZ64(uint64_t n) {
    auto fast = abel::countl_zero(n);
    auto slow = abel::countl_zero_template(n);
    EXPECT_EQ(fast, slow) << n;
    return fast;
}

int CLZ32(uint32_t n) {
    auto fast = abel::countl_zero(n);
    auto slow = abel::countl_zero_template(n);
    EXPECT_EQ(fast, slow) << n;
    return fast;
}

TEST(countl_zero, countl_zero_64) {
    EXPECT_EQ(64, CLZ64(uint64_t{}));
    EXPECT_EQ(0, CLZ64(~uint64_t{}));

    for (int index = 0; index < 64; index++) {
        uint64_t x = static_cast<uint64_t>(1) << index;
        const auto cnt = 63 - index;
        ASSERT_EQ(cnt, CLZ64(x)) << index;
        ASSERT_EQ(cnt, CLZ64(x + x - 1)) << index;
    }
}

TEST(countl_zero, countl_zero_32) {
    EXPECT_EQ(32, CLZ32(uint32_t{}));
    EXPECT_EQ(0, CLZ32(~uint32_t{}));

    for (int index = 0; index < 32; index++) {
        uint32_t x = static_cast<uint32_t>(1) << index;
        const auto cnt = 31 - index;
        ASSERT_EQ(cnt, CLZ32(x)) << index;
        ASSERT_EQ(cnt, CLZ32(x + x - 1)) << index;
        ASSERT_EQ(CLZ64(x), CLZ32(x) + 32);
    }
}
