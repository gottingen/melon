// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com


#include <string>
#include <utility>
#include "gtest/gtest.h"
#include "testing/atomic_hash_test_utils.h"
#include "abel/atomic/hash_map.h"

using Tbl = UniquePtrTable<int>;
using Uptr = std::unique_ptr<int>;

const size_t TBL_INIT = 1;
const size_t TBL_SIZE = TBL_INIT * Tbl::slot_per_bucket() * 2;

void check_key_eq(Tbl &tbl, int key, int expected_val) {
    EXPECT_TRUE(tbl.contains(Uptr(new int(key))));
    tbl.find_fn(Uptr(new int(key)), [expected_val](const Uptr &ptr) {
        EXPECT_TRUE(*ptr == expected_val);
    });
}

TEST(noncopyable, insert_and_update) {
    Tbl tbl(TBL_INIT);
    for (size_t i = 0; i < TBL_SIZE; ++i) {
        EXPECT_TRUE(tbl.insert(Uptr(new int(i)), Uptr(new int(i))));
    }
    for (size_t i = 0; i < TBL_SIZE; ++i) {
        check_key_eq(tbl, i, i);
    }
    for (size_t i = 0; i < TBL_SIZE; ++i) {
        tbl.update(Uptr(new int(i)), Uptr(new int(i + 1)));
    }
    for (size_t i = 0; i < TBL_SIZE; ++i) {
        check_key_eq(tbl, i, i + 1);
    }
}

TEST(noncopyable, upsert) {
    Tbl tbl(TBL_INIT);
    auto increment = [](Uptr &ptr) { *ptr += 1; };
    for (size_t i = 0; i < TBL_SIZE; ++i) {
        tbl.upsert(Uptr(new int(i)), increment, Uptr(new int(i)));
    }
    for (size_t i = 0; i < TBL_SIZE; ++i) {
        check_key_eq(tbl, i, i);
    }
    for (size_t i = 0; i < TBL_SIZE; ++i) {
        tbl.upsert(Uptr(new int(i)), increment, Uptr(new int(i)));
    }
    for (size_t i = 0; i < TBL_SIZE; ++i) {
        check_key_eq(tbl, i, i + 1);
    }
}

TEST(noncopyable, noncopyable_iteration) {
    Tbl tbl(TBL_INIT);
    for (size_t i = 0; i < TBL_SIZE; ++i) {
        tbl.insert(Uptr(new int(i)), Uptr(new int(i)));
    }
    {
        auto locked_tbl = tbl.lock_table();
        for (auto &kv : locked_tbl) {
            EXPECT_TRUE(*kv.first == *kv.second);
            *kv.second += 1;
        }
    }
    {
        auto locked_tbl = tbl.lock_table();
        for (auto &kv : locked_tbl) {
            EXPECT_TRUE(*kv.first == *kv.second - 1);
        }
    }
}

TEST(noncopyable, nested_table) {
    typedef abel::atomic_hash_map<char, std::string> inner_tbl;
    typedef abel::atomic_hash_map<std::string, std::unique_ptr<inner_tbl>> nested_tbl;
    nested_tbl tbl;
    std::string keys[] = {"abc", "def"};
    for (std::string &k : keys) {
        tbl.insert(std::string(k), nested_tbl::mapped_type(new inner_tbl));
        tbl.update_fn(k, [&k](nested_tbl::mapped_type &t) {
            for (char c : k) {
                t->insert(c, std::string(k));
            }
        });
    }
    for (std::string &k : keys) {
        EXPECT_TRUE(tbl.contains(k));
        tbl.update_fn(k, [&k](nested_tbl::mapped_type &t) {
            for (char c : k) {
                EXPECT_TRUE(t->find(c) == k);
            }
        });
    }
}

TEST(noncopyable, noncopyable_insert_lifetime) {
    Tbl tbl;

// Successful insert
    {
        Uptr key(new int(20));
        Uptr value(new int(20));
        EXPECT_TRUE(tbl.insert(std::move(key), std::move(value)));
        EXPECT_TRUE(!static_cast<bool>(key));
        EXPECT_TRUE(!static_cast<bool>(value));
    }

// Unsuccessful insert

    {
        tbl.insert(new int(20), new int(20));
        Uptr key(new int(20));
        Uptr value(new int(30));
        EXPECT_FALSE(tbl.insert(std::move(key), std::move(value)));
        EXPECT_TRUE(static_cast<bool>(key));
        EXPECT_TRUE(static_cast<bool>(value));
    }
}

TEST(noncopyable, noncopyable_erase_fn) {
    Tbl tbl;
    tbl.insert(new int(10), new int(10));
    auto decrement_and_erase = [](Uptr &p) {
        --(*p);
        return *p == 0;
    };
    Uptr k(new int(10));
    for (int i = 0; i < 9; ++i) {
        tbl.erase_fn(k, decrement_and_erase);
        EXPECT_TRUE(tbl.contains(k));
    }
    tbl.erase_fn(k, decrement_and_erase);
    EXPECT_FALSE(tbl.contains(k));
}

TEST(noncopyable, noncopyable_uprase_fn) {
    Tbl tbl;
    auto decrement_and_erase = [](Uptr &p) {
        --(*p);
        return *p == 0;
    };
    EXPECT_TRUE(
            tbl.uprase_fn(Uptr(new int(10)), decrement_and_erase, Uptr(new int(10))));
    Uptr k(new int(10)), v(new int(10));
    for (int i = 0; i < 10; ++i) {
        EXPECT_FALSE(
                tbl.uprase_fn(std::move(k), decrement_and_erase, std::move(v)));
        EXPECT_TRUE((k && v));
        if (i < 9) {
            EXPECT_TRUE(tbl.contains(k));
        } else {
            EXPECT_FALSE(tbl.contains(k));
        }
    }
}

