// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com


#include <array>
#include <cmath>
#include <stdexcept>
#include "testing/atomic_hash_test_utils.h"
#include "abel/atomic/hash_map.h"
#include "gtest/gtest.h"

TEST(constructor, default_size) {
    IntIntTable tbl;
    EXPECT_TRUE(tbl.size() == 0);
    EXPECT_TRUE(tbl.empty());
    if (abel::DEFAULT_SIZE < 4) {
        EXPECT_TRUE(tbl.hash_power() == 0);
    } else {
        EXPECT_TRUE(tbl.hash_power() == (size_t) log2(abel::DEFAULT_SIZE / 4));
    }
    EXPECT_TRUE(tbl.bucket_count() == 1UL << tbl.hash_power());
    EXPECT_TRUE(tbl.load_factor() == 0);
}

TEST(constructor, given_size) {
    IntIntTable tbl(1);
    EXPECT_TRUE(tbl.size() == 0);
    EXPECT_TRUE(tbl.empty());
    EXPECT_TRUE(tbl.hash_power() == 0);
    EXPECT_TRUE(tbl.bucket_count() == 1);
    EXPECT_TRUE(tbl.load_factor() == 0);
}

TEST(constructor, frees_even_with_exceptions) {
    typedef IntIntTableWithAlloc<TrackingAllocator<int, 0>> no_space_table;
// Should throw when allocating anything
    EXPECT_THROW(no_space_table(1), std::bad_alloc);
    EXPECT_TRUE(get_unfreed_bytes() == 0);

    typedef IntIntTableWithAlloc<
            TrackingAllocator<int, abel::UnitTestInternalAccess::IntIntBucketSize * 2>>
            some_space_table;
// Should throw when allocating things after the bucket
    EXPECT_THROW(some_space_table(1), std::bad_alloc);
    EXPECT_TRUE(get_unfreed_bytes() == 0);
}

struct StatefulHash {
    StatefulHash(int state_) : state(state_) {}

    size_t operator()(int x) const { return x; }

    int state;
};

struct StatefulKeyEqual {
    StatefulKeyEqual(int state_) : state(state_) {}

    bool operator()(int x, int y) const { return x == y; }

    int state;
};

template<typename T>
struct StatefulAllocator {
    using value_type = T;
    using pointer = T *;
    using const_pointer = const T *;
    using reference = T &;
    using const_reference = const T &;
    using size_type = size_t;
    using difference_type = ptrdiff_t;

    template<typename U>
    struct rebind {
        using other = StatefulAllocator<U>;
    };

    StatefulAllocator() : state(0) {}

    StatefulAllocator(int state_) : state(state_) {}

    template<typename U>
    StatefulAllocator(const StatefulAllocator<U> &other) : state(other.state) {}

    T *allocate(size_t n) { return std::allocator<T>().allocate(n); }

    void deallocate(T *p, size_t n) { std::allocator<T>().deallocate(p, n); }

    template<typename U, class... Args>
    void construct(U *p, Args &&... args) {
        new((void *) p) U(std::forward<Args>(args)...);
    }

    template<typename U>
    void destroy(U *p) { p->~U(); }

    StatefulAllocator select_on_container_copy_construction() const {
        return StatefulAllocator();
    }

    using propagate_on_container_swap = std::integral_constant<bool, true>;

    int state;
};

template<typename T, typename U>
bool operator==(const StatefulAllocator<T> &a1,
                const StatefulAllocator<U> &a2) {
    return a1.state == a2.state;
}

template<typename T, typename U>
bool operator!=(const StatefulAllocator<T> &a1,
                const StatefulAllocator<U> &a2) {
    return a1.state != a2.state;
}

using alloc_t = StatefulAllocator<std::pair<const int, int>>;
using tbl_t =
abel::atomic_hash_map<int, int, StatefulHash, StatefulKeyEqual, alloc_t, 4>;

TEST(constructor, stateful_components) {
    tbl_t map(8, StatefulHash(10), StatefulKeyEqual(20), alloc_t(30));
    EXPECT_TRUE(map.hash_function().state == 10);
    for (int i = 0; i < 100; ++i) {
        EXPECT_TRUE(map.hash_function()(i) == i);
    }
    EXPECT_TRUE(map.key_eq().state == 20);
    for (int i = 0; i < 100; ++i) {
        EXPECT_TRUE(map.key_eq()(i, i));
        EXPECT_FALSE(map.key_eq()(i, i + 1));
    }
    EXPECT_TRUE(map.get_allocator().state == 30);
}

TEST(constructor, range_constructor) {
    std::array<typename alloc_t::value_type, 3> elems{{{1, 2}, {3, 4}, {5, 6}}};
    tbl_t map(elems.begin(), elems.end(), 3, StatefulHash(10),
              StatefulKeyEqual(20), alloc_t(30));
    EXPECT_TRUE(map.hash_function().state == 10);
    EXPECT_TRUE(map.key_eq().state == 20);
    EXPECT_TRUE(map.get_allocator().state == 30);
    for (int i = 1; i <= 5; i += 2) {
        EXPECT_TRUE(map.find(i) == i + 1);
    }
}

TEST(constructor, copy_constructor) {
    tbl_t map(0, StatefulHash(10), StatefulKeyEqual(20), alloc_t(30));
    EXPECT_TRUE(map.get_allocator().state == 30);
    tbl_t map2(map);
    EXPECT_TRUE(map2.hash_function().state == 10);
    EXPECT_TRUE(map2.key_eq().state == 20);
    EXPECT_TRUE(map2.get_allocator().state == 0);
}

TEST(constructor, copy_constructor_other_allocator) {
    tbl_t map(0, StatefulHash(10), StatefulKeyEqual(20), alloc_t(30));
    tbl_t map2(map, map.get_allocator());
    EXPECT_TRUE(map2.hash_function().state == 10);
    EXPECT_TRUE(map2.key_eq().state == 20);
    EXPECT_TRUE(map2.get_allocator().state == 30);
}

TEST(constructor, move_constructor) {
    tbl_t map(10, StatefulHash(10), StatefulKeyEqual(20), alloc_t(30));
    map.insert(10, 10);
    tbl_t map2(std::move(map));
    EXPECT_TRUE(map.size() == 0);
    EXPECT_TRUE(map2.size() == 1);
    EXPECT_TRUE(map2.hash_function().state == 10);
    EXPECT_TRUE(map2.key_eq().state == 20);
    EXPECT_TRUE(map2.get_allocator().state == 30);
}

TEST(constructor, move_constructor_different_allocator) {
    tbl_t map(10, StatefulHash(10), StatefulKeyEqual(20), alloc_t(30));
    map.insert(10, 10);
    tbl_t map2(std::move(map), alloc_t(40));
    EXPECT_TRUE(map.size() == 1);
    EXPECT_TRUE(map.hash_function().state == 10);
    EXPECT_TRUE(map.key_eq().state == 20);
    EXPECT_TRUE(map.get_allocator().state == 30);

    EXPECT_TRUE(map2.size() == 1);
    EXPECT_TRUE(map2.hash_function().state == 10);
    EXPECT_TRUE(map2.key_eq().state == 20);
    EXPECT_TRUE(map2.get_allocator().state == 40);
}

TEST(constructor, initializer_list) {
    tbl_t map({{1, 2},
               {3, 4},
               {5, 6}}, 3, StatefulHash(10), StatefulKeyEqual(20),
              alloc_t(30));
    EXPECT_TRUE(map.hash_function().state == 10);
    EXPECT_TRUE(map.key_eq().state == 20);
    EXPECT_TRUE(map.get_allocator().state == 30);
    for (int i = 1; i <= 5; i += 2) {
        EXPECT_TRUE(map.find(i) == i + 1);
    }
}

TEST(constructor, swap_maps) {
    tbl_t map({{1, 2}}, 1, StatefulHash(10), StatefulKeyEqual(20), alloc_t(30));
    tbl_t map2({{3, 4}}, 1, StatefulHash(40), StatefulKeyEqual(50), alloc_t(60));
    map.swap(map2);

    EXPECT_TRUE(map.size() == 1);
    EXPECT_TRUE(map.hash_function().state == 40);
    EXPECT_TRUE(map.key_eq().state == 50);
    EXPECT_TRUE(map.get_allocator().state == 60);

    EXPECT_TRUE(map2.size() == 1);
    EXPECT_TRUE(map2.hash_function().state == 10);
    EXPECT_TRUE(map2.key_eq().state == 20);
    EXPECT_TRUE(map2.get_allocator().state == 30);

// Uses ADL to find the specialized swap.
    swap(map, map2);

    EXPECT_TRUE(map.size() == 1);
    EXPECT_TRUE(map.hash_function().state == 10);
    EXPECT_TRUE(map.key_eq().state == 20);
    EXPECT_TRUE(map.get_allocator().state == 30);

    EXPECT_TRUE(map2.size() == 1);
    EXPECT_TRUE(map2.hash_function().state == 40);
    EXPECT_TRUE(map2.key_eq().state == 50);
    EXPECT_TRUE(map2.get_allocator().state == 60);
}

TEST(constructor, copy_assign_different_allocators) {
    tbl_t map({{1, 2}}, 1, StatefulHash(10), StatefulKeyEqual(20), alloc_t(30));
    tbl_t map2({{3, 4}}, 1, StatefulHash(40), StatefulKeyEqual(50), alloc_t(60));

    map = map2;
    EXPECT_TRUE(map.size() == 1);
    EXPECT_TRUE(map.find(3) == 4);
    EXPECT_TRUE(map.hash_function().state == 40);
    EXPECT_TRUE(map.key_eq().state == 50);
    EXPECT_TRUE(map.get_allocator().state == 30);

    EXPECT_TRUE(map2.size() == 1);
    EXPECT_TRUE(map2.hash_function().state == 40);
    EXPECT_TRUE(map2.key_eq().state == 50);
    EXPECT_TRUE(map2.get_allocator().state == 60);
}

TEST(constructor, move_assign_different_allocators) {
    tbl_t map({{1, 2}}, 1, StatefulHash(10), StatefulKeyEqual(20), alloc_t(30));
    tbl_t map2({{3, 4}}, 1, StatefulHash(40), StatefulKeyEqual(50), alloc_t(60));

    map = std::move(map2);
    EXPECT_TRUE(map.size() == 1);
    EXPECT_TRUE(map.find(3) == 4);
    EXPECT_TRUE(map.hash_function().state == 40);
    EXPECT_TRUE(map.key_eq().state == 50);
    EXPECT_TRUE(map.get_allocator().state == 30);

    EXPECT_TRUE(map2.hash_function().state == 40);
    EXPECT_TRUE(map2.key_eq().state == 50);
    EXPECT_TRUE(map2.get_allocator().state == 60);
}

TEST(constructor, move_assign_same_allocators) {
    tbl_t map({{1, 2}}, 1, StatefulHash(10), StatefulKeyEqual(20), alloc_t(30));
    tbl_t map2({{3, 4}}, 1, StatefulHash(40), StatefulKeyEqual(50), alloc_t(30));

    map = std::move(map2);
    EXPECT_TRUE(map.size() == 1);
    EXPECT_TRUE(map.find(3) == 4);
    EXPECT_TRUE(map.hash_function().state == 40);
    EXPECT_TRUE(map.key_eq().state == 50);
    EXPECT_TRUE(map.get_allocator().state == 30);

    EXPECT_TRUE(map2.size() == 0);
    EXPECT_TRUE(map2.hash_function().state == 40);
    EXPECT_TRUE(map2.key_eq().state == 50);
    EXPECT_TRUE(map2.get_allocator().state == 30);
}

TEST(constructor, initializer_list_assignment) {
    tbl_t map({{1, 2}}, 1, StatefulHash(10), StatefulKeyEqual(20), alloc_t(30));
    EXPECT_TRUE(map.find(1) == 2);
    map = {{3, 4}};
    EXPECT_TRUE(map.find(3) == 4);
}