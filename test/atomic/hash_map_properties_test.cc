// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com


#include "gtest/gtest.h"
#include "testing/atomic_hash_test_utils.h"
#include "abel/atomic/hash_map.h"

using abel::UnitTestInternalAccess;

// Checks that the alt index function returns a different bucket, and can
// recover the old bucket when called with the alternate bucket as the index.
template<class CuckoohashMap>
void check_key(size_t hash_power, const typename CuckoohashMap::key_type &key) {
    auto hashfn = typename CuckoohashMap::hasher();
    size_t hv = hashfn(key);
    auto partial = UnitTestInternalAccess::partial_key<CuckoohashMap>(hv);
    size_t bucket =
            UnitTestInternalAccess::index_hash<CuckoohashMap>(hash_power, hv);
    size_t alt_bucket = UnitTestInternalAccess::alt_index<CuckoohashMap>(
            hash_power, partial, bucket);
    size_t orig_bucket = UnitTestInternalAccess::alt_index<CuckoohashMap>(
            hash_power, partial, alt_bucket);

    EXPECT_TRUE(bucket != alt_bucket);
    EXPECT_TRUE(bucket == orig_bucket);
}

TEST(hash_properties, int_alt_index_works_correctly) {
    for (size_t hash_power = 10; hash_power < 15; ++hash_power) {
        for (int key = 0; key < 10000; ++key) {
            check_key<IntIntTable>(hash_power, key);
        }
    }
}

TEST(hash_properties, string_alt_index) {
    for (size_t hash_power = 10; hash_power < 15; ++hash_power) {
        for (int key = 0; key < 10000; ++key) {
            check_key<StringIntTable>(hash_power, std::to_string(key));
        }
    }
}

TEST(hash_properties, larger_hashpower) {
    std::string key = "abc";
    size_t hv = StringIntTable::hasher()(key);
    for (size_t hash_power = 1;hash_power < 30; ++hash_power) {
        auto partial = UnitTestInternalAccess::partial_key<StringIntTable>(hv);
        size_t index_bucket1 =
                UnitTestInternalAccess::index_hash<StringIntTable>(hash_power, hv);
        size_t index_bucket2 =
                UnitTestInternalAccess::index_hash<StringIntTable>(hash_power + 1, hv);
        EXPECT_TRUE((index_bucket2 & ~(1L << hash_power)) == index_bucket1);

        size_t alt_bucket1 = UnitTestInternalAccess::alt_index<StringIntTable>(
                hash_power, partial, index_bucket1);
        size_t alt_bucket2 = UnitTestInternalAccess::alt_index<StringIntTable>(
                hash_power, partial, index_bucket2);

        EXPECT_TRUE((alt_bucket2 & ~(1L << hash_power)) == alt_bucket1);
    }
}
