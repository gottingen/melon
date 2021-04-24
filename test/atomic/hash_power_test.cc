// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com


#include "testing/atomic_hash_test_utils.h"
#include "abel/atomic/hash_map.h"
#include "abel/atomic/hash_config.h"
#include "gtest/gtest.h"

TEST(max_power, init_default) {
    IntIntTable tbl;
    EXPECT_TRUE(tbl.maximum_hash_power() == abel::NO_MAXIMUM_HASHPOWER);
}

TEST(max_power, expansion) {
    IntIntTable tbl(1);
    tbl.maximum_hash_power(1);
    for (size_t i = 0; i < 2 * tbl.slot_per_bucket(); ++i) {
        tbl.insert(i, i);
    }

    EXPECT_TRUE(tbl.hash_power() == 1);
    EXPECT_THROW(tbl.insert(2 * tbl.slot_per_bucket(), 0),
                 abel::maximum_hashpower_exceeded);
    EXPECT_THROW(tbl.rehash(2), abel::maximum_hashpower_exceeded);
    EXPECT_THROW(tbl.reserve(4 * tbl.slot_per_bucket()),
                 abel::maximum_hashpower_exceeded);
}

TEST(max_power, hash_power) {
// It's difficult to check that we actually don't ever set a maximum hash
// power, but if we explicitly unset it, we should be able to expand beyond
// the limit that we had previously set.
    IntIntTable tbl(1);
    tbl.maximum_hash_power(1);
    EXPECT_THROW(tbl.rehash(2), abel::maximum_hashpower_exceeded);

    tbl.maximum_hash_power(2);
    tbl.rehash(2);
    EXPECT_TRUE(tbl.hash_power() == 2);
    EXPECT_THROW(tbl.rehash(3), abel::maximum_hashpower_exceeded);

    tbl.maximum_hash_power(abel::NO_MAXIMUM_HASHPOWER);
    tbl.rehash(10);
    EXPECT_TRUE(tbl.hash_power() == 10);
}


TEST(max_power, factor) {
    IntIntTable tbl;
    EXPECT_TRUE(tbl.minimum_load_factor() == abel::DEFAULT_MINIMUM_LOAD_FACTOR);
}

class BadHashFunction {
public:
    size_t operator()(int) { return 0; }
};

TEST(max_power, caps_automatic_expansion) {
    const size_t slot_per_bucket = 4;
    abel::atomic_hash_map<int, int, BadHashFunction, std::equal_to<int>,
            std::allocator<std::pair<const int, int>>, slot_per_bucket>
            tbl(16);
    tbl.minimum_load_factor(0.6);

    for (size_t i = 0; i < 2 * slot_per_bucket; ++i) {
        tbl.insert(i, i);
    }

    EXPECT_THROW(tbl.insert(2 * slot_per_bucket, 0),
                 abel::load_factor_too_low);
}

TEST(max_power, invalid_minimum) {
    IntIntTable tbl;
    EXPECT_THROW(tbl.minimum_load_factor(-0.01), std::invalid_argument);
    EXPECT_THROW(tbl.minimum_load_factor(1.01), std::invalid_argument);
}
