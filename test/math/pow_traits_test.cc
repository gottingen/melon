// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com

#include <limits>
#include "math_test.h"
#include "abel/math/pow_traits.h"

TEST(is_power_of_two, all) {

    using abel::is_power_of_two;

    EXPECT_TRUE(is_power_of_two(uint8_t{0}));
    EXPECT_TRUE(is_power_of_two(uint8_t{1}));
    EXPECT_TRUE(is_power_of_two(uint8_t{2}));
    EXPECT_FALSE(is_power_of_two(uint8_t{3}));
    EXPECT_TRUE(is_power_of_two(uint8_t{16}));
    EXPECT_FALSE(is_power_of_two(uint8_t{17}));
    EXPECT_FALSE(is_power_of_two((std::numeric_limits<uint8_t>::max)()));

    EXPECT_TRUE(is_power_of_two(uint16_t{0}));
    EXPECT_TRUE(is_power_of_two(uint16_t{1}));
    EXPECT_TRUE(is_power_of_two(uint16_t{2}));
    EXPECT_FALSE(is_power_of_two(uint16_t{3}));
    EXPECT_TRUE(is_power_of_two(uint16_t{16}));
    EXPECT_FALSE(is_power_of_two(uint16_t{17}));
    EXPECT_FALSE(is_power_of_two((std::numeric_limits<uint16_t>::max)()));

    EXPECT_TRUE(is_power_of_two(uint32_t{0}));
    EXPECT_TRUE(is_power_of_two(uint32_t{1}));
    EXPECT_TRUE(is_power_of_two(uint32_t{2}));
    EXPECT_FALSE(is_power_of_two(uint32_t{3}));
    EXPECT_TRUE(is_power_of_two(uint32_t{32}));
    EXPECT_FALSE(is_power_of_two(uint32_t{17}));
    EXPECT_FALSE(is_power_of_two((std::numeric_limits<uint32_t>::max)()));

    EXPECT_TRUE(is_power_of_two(uint64_t{0}));
    EXPECT_TRUE(is_power_of_two(uint64_t{1}));
    EXPECT_TRUE(is_power_of_two(uint64_t{2}));
    EXPECT_FALSE(is_power_of_two(uint64_t{3}));
    EXPECT_TRUE(is_power_of_two(uint64_t{64}));
    EXPECT_FALSE(is_power_of_two(uint64_t{17}));
    EXPECT_FALSE(is_power_of_two((std::numeric_limits<uint64_t>::max)()));

}
