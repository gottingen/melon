// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "abel/base/profile.h"
#include "abel/atomic/atomic_bitset.h"

TEST(atomic_bitset, Simple) {
    constexpr size_t kSize = 1000;
    abel::atomic_bitset<kSize> bs;

    EXPECT_EQ(kSize, bs.size());

    for (size_t i = 0; i < kSize; ++i) {
        EXPECT_FALSE(bs[i]);
    }

    bs.set(42);
    for (size_t i = 0; i < kSize; ++i) {
        EXPECT_EQ(i == 42, bs[i]);
    }

    bs.set(43);
    for (size_t i = 0; i < kSize; ++i) {
        EXPECT_EQ((i == 42 || i == 43), bs[i]);
    }

    bs.reset(42);
    for (size_t i = 0; i < kSize; ++i) {
        EXPECT_EQ((i == 43), bs[i]);
    }

    bs.reset(43);
    for (size_t i = 0; i < kSize; ++i) {
        EXPECT_FALSE(bs[i]);
    }

    bs.set(268);
    for (size_t i = 0; i < kSize; ++i) {
        EXPECT_EQ(i == 268, bs[i]);
    }

    bs.reset(268);
    for (size_t i = 0; i < kSize; ++i) {
        EXPECT_FALSE(bs[i]);
    }
}