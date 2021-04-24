// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com

#include <array>
#include "gtest/gtest.h"
#include "testing/atomic_hash_test_utils.h"
#include "abel/atomic/hash_map.h"

using abel::UnitTestInternalAccess;

TEST(resize, rehash_empty_table) {
    IntIntTable table(1);
    EXPECT_TRUE(table.hash_power() == 0);

    table.rehash(20);
    EXPECT_TRUE(table.hash_power() == 20);

    table.rehash(1);
    EXPECT_TRUE(table.hash_power() == 1);
}

TEST(resize, reserve_empty_table) {
    IntIntTable table(1);
    table.reserve(100);
    EXPECT_TRUE(table.hash_power() == 5);

    table.reserve(1);
    EXPECT_TRUE(table.hash_power() == 0);

    table.reserve(2);
    EXPECT_TRUE(table.hash_power() == 0);
}

TEST(resize, reserve_calc) {
    const size_t slot_per_bucket = IntIntTable::slot_per_bucket();
    EXPECT_TRUE(UnitTestInternalAccess::reserve_calc<IntIntTable>(0) == 0);
    EXPECT_TRUE(UnitTestInternalAccess::reserve_calc<IntIntTable>(
            1 * slot_per_bucket) == 0);

    EXPECT_TRUE(UnitTestInternalAccess::reserve_calc<IntIntTable>(
            2 * slot_per_bucket) == 1);
    EXPECT_TRUE(UnitTestInternalAccess::reserve_calc<IntIntTable>(
            3 * slot_per_bucket) == 2);
    EXPECT_TRUE(UnitTestInternalAccess::reserve_calc<IntIntTable>(
            4 * slot_per_bucket) == 2);
    EXPECT_TRUE(UnitTestInternalAccess::reserve_calc<IntIntTable>(
            2500000 * slot_per_bucket) == 22);

    EXPECT_TRUE(UnitTestInternalAccess::reserve_calc<IntIntTable>(
            (1ULL << 31) * slot_per_bucket) == 31);
    EXPECT_TRUE(UnitTestInternalAccess::reserve_calc<IntIntTable>(
            ((1ULL << 31) + 1) * slot_per_bucket) == 32);

    EXPECT_TRUE(UnitTestInternalAccess::reserve_calc<IntIntTable>(
            (1ULL << 61) * slot_per_bucket) == 61);
    EXPECT_TRUE(UnitTestInternalAccess::reserve_calc<IntIntTable>(
            ((1ULL << 61) + 1) * slot_per_bucket) == 62);
}

struct my_type {
    int x;

    my_type(int v) : x(v) {}

    my_type(const my_type &other) {
        x = other.x;
    }

    my_type(my_type &&other) {
        x = other.x;
        ++num_moves;
    }

    ~my_type() { ++num_deletes; }

    static size_t num_deletes;
    static size_t num_moves;
};

size_t my_type::num_deletes = 0;
size_t my_type::num_moves = 0;

TEST(resize, Resizing_number_of_frees) {
    my_type val(0);
    {
        // Should allocate 2 buckets of 4 slots
        abel::atomic_hash_map<int, my_type, std::hash<int>, std::equal_to<int>,
                std::allocator<std::pair<const int, my_type>>, 4>
                map(8);
        for (int i = 0; i < 9; ++i) {
            map.insert(i, val);
        }
        // All of the items should be moved during resize to the new region of
        // memory. They should be deleted from the old container.
        EXPECT_TRUE(my_type::num_deletes == 8);
        EXPECT_TRUE(my_type::num_moves == 8);
    }
    EXPECT_TRUE(my_type::num_deletes == 17);
}

// Taken from https://github.com/facebook/folly/blob/master/folly/docs/Traits.md
class NonRelocatableType {
public:
    std::array<char, 1024> buffer;
    char *pointerToBuffer;

    NonRelocatableType() : pointerToBuffer(buffer.data()) {}

    NonRelocatableType(char c) : pointerToBuffer(buffer.data()) {
        buffer.fill(c);
    }

    NonRelocatableType(const NonRelocatableType &x) noexcept
            : buffer(x.buffer), pointerToBuffer(buffer.data()) {}

    NonRelocatableType &operator=(const NonRelocatableType &x) {
        buffer = x.buffer;
        return *this;
    }
};

TEST(resize, Resize_on_non_relocatable_type) {
    abel::atomic_hash_map<int, NonRelocatableType, std::hash<int>, std::equal_to<int>,
            std::allocator<std::pair<const int, NonRelocatableType>>, 1>
            map(0);
    EXPECT_TRUE(map.hash_power() == 0);
    // Make it resize a few times to ensure the vector capacity has to actually
    // change when we resize the buckets
    const size_t num_elems = 16;
    for (int i = 0; i < num_elems; ++i) {
        map.insert(i, 'a');
    }
    // Make sure each pointer actually points to its buffer
    NonRelocatableType value;
    std::array<char, 1024> ref;
    ref.fill('a');
    auto lt = map.lock_table();
    for (const auto &kvpair: lt) {
        EXPECT_TRUE(ref == kvpair.second.buffer);
        EXPECT_TRUE(kvpair.second.pointerToBuffer == kvpair.second.buffer.data());
    }
}
