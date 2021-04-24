// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com

#include "gtest/gtest.h"
#include "abel/atomic/hash_map.h"

size_t int_constructions;
size_t copy_constructions;
size_t destructions;
size_t foo_comparisons;
size_t int_comparisons;
size_t foo_hashes;
size_t int_hashes;

class Foo {
public:
    int val;

    Foo(int v) {
        ++int_constructions;
        val = v;
    }

    Foo(const Foo &x) {
        ++copy_constructions;
        val = x.val;
    }

    ~Foo() { ++destructions; }
};

class foo_eq {
public:
    bool operator()(const Foo &left, const Foo &right) const {
        ++foo_comparisons;
        return left.val == right.val;
    }

    bool operator()(const Foo &left, const int right) const {
        ++int_comparisons;
        return left.val == right;
    }
};

class foo_hasher {
public:
    size_t operator()(const Foo &x) const {
        ++foo_hashes;
        return static_cast<size_t>(x.val);
    }

    size_t operator()(const int x) const {
        ++int_hashes;
        return static_cast<size_t>(x);
    }
};

typedef abel::atomic_hash_map<Foo, bool, foo_hasher, foo_eq> foo_map;

void init_vars() {
// setup code
    int_constructions = 0;
    copy_constructions = 0;
    destructions = 0;
    foo_comparisons = 0;
    int_comparisons = 0;
    foo_hashes = 0;
    int_hashes = 0;
}

TEST(hash_compare, insert) {
    init_vars();
    {
        foo_map map;
        map.insert(0, true);
    }
    EXPECT_TRUE(int_constructions == 1);
    EXPECT_TRUE(copy_constructions == 0);
    EXPECT_TRUE(destructions == 1);
    EXPECT_TRUE(foo_comparisons == 0);
    EXPECT_TRUE(int_comparisons == 0);
    EXPECT_TRUE(foo_hashes == 0);
    EXPECT_TRUE(int_hashes == 1);
}

TEST(hash_compare, foo_insert) {
    init_vars();
    {
        foo_map map;
        map.insert(Foo(0), true);
    }
    EXPECT_TRUE(int_constructions == 1);
    EXPECT_TRUE(copy_constructions == 1);
// One destruction of passed-in and moved argument, and one after the
// table is destroyed.
    EXPECT_TRUE(destructions == 2);
    EXPECT_TRUE(foo_comparisons == 0);
    EXPECT_TRUE(int_comparisons == 0);
    EXPECT_TRUE(foo_hashes == 1);
    EXPECT_TRUE(int_hashes == 0);
}

TEST(hash_compare, insert_or_assign) {
    init_vars();
    {
        foo_map map;
        map.insert_or_assign(0, true);
        map.insert_or_assign(0, false);
        EXPECT_FALSE(map.find(0));
    }
    EXPECT_TRUE(int_constructions == 1);
    EXPECT_TRUE(copy_constructions == 0);
    EXPECT_TRUE(destructions == 1);
    EXPECT_TRUE(foo_comparisons == 0);
    EXPECT_TRUE(int_comparisons == 2);
    EXPECT_TRUE(foo_hashes == 0);
    EXPECT_TRUE(int_hashes == 3);
}

TEST(hash_compare, foo_insert_or_assign) {
    init_vars();
    {
        foo_map map;
        map.insert_or_assign(Foo(0),true);
        map.insert_or_assign(Foo(0),false);
        EXPECT_FALSE(map.find(Foo(0)));
    }
    EXPECT_TRUE(int_constructions == 3);
    EXPECT_TRUE(copy_constructions == 1);
// Three destructions of Foo arguments, and one in table destruction
    EXPECT_TRUE(destructions == 4);
    EXPECT_TRUE(foo_comparisons == 2);
    EXPECT_TRUE(int_comparisons == 0);
    EXPECT_TRUE(foo_hashes == 3);
    EXPECT_TRUE(int_hashes == 0);
}

TEST(hash_compare, find) {
    init_vars();
    {
        foo_map map;
        map.insert(0, true);
        bool val;
        map.find(0, val);
        EXPECT_TRUE(val);
        EXPECT_TRUE(map.find(0, val) == true);
        EXPECT_TRUE(map.find(1, val) == false);
    }
    EXPECT_TRUE(int_constructions == 1);
    EXPECT_TRUE(copy_constructions == 0);
    EXPECT_TRUE(destructions == 1);
    EXPECT_TRUE(foo_comparisons == 0);
    EXPECT_TRUE(int_comparisons == 2);
    EXPECT_TRUE(foo_hashes == 0);
    EXPECT_TRUE(int_hashes == 4);
}

TEST(hash_compare, foo_find) {
    init_vars();
    {
        foo_map map;
        map.insert(0, true);
        bool val;
        map.find(Foo(0), val);
        EXPECT_TRUE(val);
        EXPECT_TRUE(map.find(Foo(0), val) == true);
        EXPECT_TRUE(map.find(Foo(1), val) == false);
    }
    EXPECT_TRUE(int_constructions == 4);
    EXPECT_TRUE(copy_constructions == 0);
    EXPECT_TRUE(destructions == 4);
    EXPECT_TRUE(foo_comparisons == 2);
    EXPECT_TRUE(int_comparisons == 0);
    EXPECT_TRUE(foo_hashes == 3);
    EXPECT_TRUE(int_hashes == 1);
}

TEST(hash_compare, contains) {
    init_vars();
    {
        foo_map map(0);
        map.rehash(2);
        map.insert(0, true);
        EXPECT_TRUE(map.contains(0));
// Shouldn't do comparison because of different partial key
        EXPECT_TRUE(!map.contains(4));
    }
    EXPECT_TRUE(int_constructions == 1);
    EXPECT_TRUE(copy_constructions == 0);
    EXPECT_TRUE(destructions == 1);
    EXPECT_TRUE(foo_comparisons == 0);
    EXPECT_TRUE(int_comparisons == 1);
    EXPECT_TRUE(foo_hashes == 0);
    EXPECT_TRUE(int_hashes == 3);
}

TEST(hash_compare, erase) {
    init_vars();
    {
        foo_map map;
        map.insert(0, true);
        EXPECT_TRUE(map.erase(0));
        EXPECT_TRUE(!map.contains(0));
    }
    EXPECT_TRUE(int_constructions == 1);
    EXPECT_TRUE(copy_constructions == 0);
    EXPECT_TRUE(destructions == 1);
    EXPECT_TRUE(foo_comparisons == 0);
    EXPECT_TRUE(int_comparisons == 1);
    EXPECT_TRUE(foo_hashes == 0);
    EXPECT_TRUE(int_hashes == 3);
}

TEST(hash_compare, update) {
    init_vars();
    {
        foo_map map;
        map.insert(0, true);
        EXPECT_TRUE(map.update(0, false));
        EXPECT_TRUE(!map.find(0));
    }
    EXPECT_TRUE(int_constructions == 1);
    EXPECT_TRUE(copy_constructions == 0);
    EXPECT_TRUE(destructions == 1);
    EXPECT_TRUE(foo_comparisons == 0);
    EXPECT_TRUE(int_comparisons == 2);
    EXPECT_TRUE(foo_hashes == 0);
    EXPECT_TRUE(int_hashes == 3);
}

TEST(hash_compare, update_fn) {
    init_vars();
    {
        foo_map map;
        map.insert(0, true);
        EXPECT_TRUE(map.update_fn(0, [](bool &val) { val = !val; }));
        EXPECT_TRUE(!map.find(0));
    }
    EXPECT_TRUE(int_constructions == 1);
    EXPECT_TRUE(copy_constructions == 0);
    EXPECT_TRUE(destructions == 1);
    EXPECT_TRUE(foo_comparisons == 0);
    EXPECT_TRUE(int_comparisons == 2);
    EXPECT_TRUE(foo_hashes == 0);
    EXPECT_TRUE(int_hashes == 3);
}

TEST(hash_compare, upsert) {
    init_vars();
    {
        foo_map map(0);
        map.rehash(2);
        auto neg = [](bool &val) { val = !val; };
        map.upsert(0, neg, true);
        map.upsert(0, neg, true);
// Shouldn't do comparison because of different partial key
        map.upsert(4, neg, false);
        EXPECT_TRUE(!map.find(0));
        EXPECT_TRUE(!map.find(4));
    }
    EXPECT_TRUE(int_constructions == 2);
    EXPECT_TRUE(copy_constructions == 0);
    EXPECT_TRUE(destructions == 2);
    EXPECT_TRUE(foo_comparisons == 0);
    EXPECT_TRUE(int_comparisons == 3);
    EXPECT_TRUE(foo_hashes == 0);
    EXPECT_TRUE(int_hashes == 5);
}

TEST(hash_compare, uprase_fn) {
    init_vars();
    {
        foo_map map(0);
        map.rehash(2);
        auto fn = [](bool &val) {
            val = !val;
            return val;
        };
        EXPECT_TRUE(map.uprase_fn(0, fn, true));
        EXPECT_FALSE(map.uprase_fn(0, fn, true));
        EXPECT_TRUE(map.contains(0));
        EXPECT_FALSE(map.uprase_fn(0, fn, true));
        EXPECT_FALSE(map.contains(0));
    }
    EXPECT_TRUE(int_constructions == 1);
    EXPECT_TRUE(copy_constructions == 0);
    EXPECT_TRUE(destructions == 1);
    EXPECT_TRUE(foo_comparisons == 0);
    EXPECT_TRUE(int_comparisons == 3);
    EXPECT_TRUE(foo_hashes == 0);
    EXPECT_TRUE(int_hashes == 5);
}

