// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com


#include <cstddef>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include "gtest/gtest.h"
#include "abel/atomic/bucket_container.h"

template<bool PROPAGATE_COPY_ASSIGNMENT = true,
        bool PROPAGATE_MOVE_ASSIGNMENT = true, bool PROPAGATE_SWAP = true>
struct allocator_wrapper {
    template<class T>
    class stateful_allocator {
    public:
        using value_type = T;
        using propagate_on_container_copy_assignment =
        std::integral_constant<bool, PROPAGATE_COPY_ASSIGNMENT>;
        using propagate_on_container_move_assignment =
        std::integral_constant<bool, PROPAGATE_MOVE_ASSIGNMENT>;
        using propagate_on_container_swap =
        std::integral_constant<bool, PROPAGATE_SWAP>;

        stateful_allocator() : id(0) {}

        stateful_allocator(const size_t &id_) : id(id_) {}

        stateful_allocator(const stateful_allocator &other) : id(other.id) {}

        template<class U>
        stateful_allocator(const stateful_allocator<U> &other) : id(other.id) {}

        stateful_allocator &operator=(const stateful_allocator &a) {
            id = a.id + 1;
            return *this;
        }

        stateful_allocator &operator=(stateful_allocator &&a) {
            id = a.id + 2;
            return *this;
        }

        T *allocate(size_t n) { return std::allocator<T>().allocate(n); }

        void deallocate(T *ptr, size_t n) {
            std::allocator<T>().deallocate(ptr, n);
        }

        stateful_allocator select_on_container_copy_construction() const {
            stateful_allocator copy(*this);
            ++copy.id;
            return copy;
        }

        bool operator==(const stateful_allocator &other) { return id == other.id; }

        bool operator!=(const stateful_allocator &other) { return id != other.id; }

        size_t id;
    };
};

template<class T, bool PCA = true, bool PMA = true>
using stateful_allocator =
typename allocator_wrapper<PCA, PMA>::template stateful_allocator<T>;

const size_t SLOT_PER_BUCKET = 4;

template<class Alloc>
using TestingContainer =
abel::bucket_container<std::shared_ptr<int>, int, Alloc, uint8_t,
        SLOT_PER_BUCKET>;

using value_type = std::pair<const std::shared_ptr<int>, int>;

TEST(bucket_container, ctor) {
    allocator_wrapper<>::stateful_allocator<value_type> a;
    TestingContainer<decltype(a)> tc(2, a);
    EXPECT_TRUE(tc.hash_power() == 2);
    EXPECT_TRUE(tc.size() == 4);
    EXPECT_TRUE(tc.get_allocator().id == 0);
    for (size_t i = 0; i < tc.size(); ++i) {
        for (size_t j = 0; j < SLOT_PER_BUCKET; ++j) {
            EXPECT_FALSE(tc[i].occupied(j));
        }
    }
}

TEST(bucket_container, allocator) {
    allocator_wrapper<>::stateful_allocator<value_type> a(10);
    TestingContainer<decltype(a)> tc(2, a);
    EXPECT_TRUE(tc.hash_power() == 2);
    EXPECT_TRUE(tc.size() == 4);
    EXPECT_TRUE(tc.get_allocator().id == 10);
}

TEST(bucket_container, copy_ctor) {
    allocator_wrapper<>::stateful_allocator<value_type> a(5);
    TestingContainer<decltype(a)> tc(2, a);
    tc.setKV(0, 0, 2, std::make_shared<int>(10), 5);
    TestingContainer<decltype(a)> tc2(tc);

    EXPECT_TRUE(tc[0].occupied(0));
    EXPECT_TRUE(tc[0].partial(0) == 2);
    EXPECT_TRUE(*tc[0].key(0) == 10);
    EXPECT_TRUE(tc[0].mapped(0) == 5);
    EXPECT_TRUE(tc.get_allocator().id == 5);

    EXPECT_TRUE(tc2[0].occupied(0));
    EXPECT_TRUE(tc2[0].partial(0) == 2);
    EXPECT_TRUE(*tc2[0].key(0) == 10);
    EXPECT_TRUE(tc2[0].mapped(0) == 5);
    EXPECT_TRUE(tc2.get_allocator().id == 6);
}

TEST(bucket_container, move_ctor) {
    allocator_wrapper<>::stateful_allocator<value_type> a(5);
    TestingContainer<decltype(a)> tc(2, a);
    tc.setKV(0, 0, 2, std::make_shared<int>(10), 5);
    TestingContainer<decltype(a)> tc2(std::move(tc));

    EXPECT_TRUE(tc2[0].occupied(0));
    EXPECT_TRUE(tc2[0].partial(0) == 2);
    EXPECT_TRUE(*tc2[0].key(0) == 10);
    EXPECT_TRUE(tc2[0].mapped(0) == 5);
    EXPECT_TRUE(tc2.get_allocator().id == 5);
}

TEST(bucket_container, copy_assigned) {
    allocator_wrapper<true>::stateful_allocator<value_type> a(5);
    TestingContainer<decltype(a)> tc(2, a);
    tc.setKV(0, 0, 2, std::make_shared<int>(10), 5);
    TestingContainer<decltype(a)> tc2(2, a);
    tc2.setKV(1, 0, 2, std::make_shared<int>(10), 5);

    tc2 = tc;
    EXPECT_TRUE(tc2[0].occupied(0));
    EXPECT_TRUE(tc2[0].partial(0) == 2);
    EXPECT_TRUE(*tc2[0].key(0) == 10);
    EXPECT_TRUE(tc2[0].key(0).use_count() == 2);
    EXPECT_TRUE(tc2[0].mapped(0) == 5);
    EXPECT_FALSE(tc2[1].occupied(0));

    EXPECT_TRUE(tc.get_allocator().id == 5);
    EXPECT_TRUE(tc2.get_allocator().id == 6);
}

TEST(bucket_container, copy_assign) {
    allocator_wrapper<false>::stateful_allocator<value_type> a(5);
    TestingContainer<decltype(a)> tc(2, a);
    tc.setKV(0, 0, 2, std::make_shared<int>(10), 5);
    TestingContainer<decltype(a)> tc2(2, a);
    tc2.setKV(1, 0, 2, std::make_shared<int>(10), 5);

    tc2 = tc;
    EXPECT_TRUE(tc2[0].occupied(0));
    EXPECT_TRUE(tc2[0].partial(0) == 2);
    EXPECT_TRUE(*tc2[0].key(0) == 10);
    EXPECT_TRUE(tc2[0].key(0).use_count() == 2);
    EXPECT_TRUE(tc2[0].mapped(0) == 5);
    EXPECT_FALSE(tc2[1].occupied(0));

    EXPECT_TRUE(tc.get_allocator().id == 5);
    EXPECT_TRUE(tc2.get_allocator().id == 5);
}

TEST(bucket_container, mobve_assign_yes) {
    allocator_wrapper<>::stateful_allocator<value_type> a(5);
    TestingContainer<decltype(a)> tc(2, a);
    tc.setKV(0, 0, 2, std::make_shared<int>(10), 5);
    TestingContainer<decltype(a)> tc2(2, a);
    tc2.setKV(1, 0, 2, std::make_shared<int>(10), 5);

    tc2 = std::move(tc);
    EXPECT_TRUE(tc2[0].occupied(0));
    EXPECT_TRUE(tc2[0].partial(0) == 2);
    EXPECT_TRUE(*tc2[0].key(0) == 10);
    EXPECT_TRUE(tc2[0].mapped(0) == 5);
    EXPECT_FALSE(tc2[1].occupied(0));
    EXPECT_TRUE(tc2.get_allocator().id == 7);
}

TEST(bucket_container, move_assign_no) {
    allocator_wrapper<true, false>::stateful_allocator<value_type> a(5);
    TestingContainer<decltype(a)> tc(2, a);
    tc.setKV(0, 0, 2, std::make_shared<int>(10), 5);
    TestingContainer<decltype(a)> tc2(2, a);
    tc2.setKV(1, 0, 2, std::make_shared<int>(10), 5);

    tc2 = std::move(tc);
    EXPECT_TRUE(tc2[0].occupied(0));
    EXPECT_TRUE(tc2[0].partial(0) == 2);
    EXPECT_TRUE(*tc2[0].key(0) == 10);
    EXPECT_TRUE(tc2[0].key(0).use_count() == 1);
    EXPECT_TRUE(tc2[0].mapped(0) == 5);
    EXPECT_FALSE(tc2[1].occupied(0));
    EXPECT_TRUE(tc2.get_allocator().id == 5);
}

TEST(bucket_container, swap_warp) {
    allocator_wrapper<true, false>::stateful_allocator<value_type> a(5);
    TestingContainer<decltype(a)> tc(2, a);
    tc.setKV(0, 0, 2, std::make_shared<int>(10), 5);
    allocator_wrapper<true, false>::stateful_allocator<value_type> a2(4);
    TestingContainer<decltype(a)> tc2(2, a2);
    tc2.setKV(1, 0, 2, std::make_shared<int>(10), 5);

    tc2 = std::move(tc);
    EXPECT_TRUE(!tc2[1].occupied(0));
    EXPECT_TRUE(tc2[0].occupied(0));
    EXPECT_TRUE(tc2[0].partial(0) == 2);
    EXPECT_TRUE(*tc2[0].key(0) == 10);
    EXPECT_TRUE(tc2[0].key(0).use_count() == 1);
    EXPECT_TRUE(tc2[0].mapped(0) == 5);
    EXPECT_FALSE(tc2[1].occupied(0));
    EXPECT_TRUE(tc2.get_allocator().id == 4);

    EXPECT_TRUE(tc[0].occupied(0));
    EXPECT_TRUE(tc[0].partial(0) == 2);
    EXPECT_FALSE(tc[0].key(0));
}

TEST(bucket_container, swap_no_p) {
    allocator_wrapper<true, true, false>::stateful_allocator<value_type> a(5);
    TestingContainer<decltype(a)> tc(2, a);
    tc.setKV(0, 0, 2, std::make_shared<int>(10), 5);
    TestingContainer<decltype(a)> tc2(2, a);
    tc2.setKV(1, 0, 2, std::make_shared<int>(10), 5);

    tc.swap(tc2);

    EXPECT_TRUE(tc[1].occupied(0));
    EXPECT_TRUE(tc[1].partial(0) == 2);
    EXPECT_TRUE(*tc[1].key(0) == 10);
    EXPECT_TRUE(tc[1].key(0).use_count() == 1);
    EXPECT_TRUE(tc[1].mapped(0) == 5);
    EXPECT_TRUE(tc.get_allocator().id == 5);

    EXPECT_TRUE(tc2[0].occupied(0));
    EXPECT_TRUE(tc2[0].partial(0) == 2);
    EXPECT_TRUE(*tc2[0].key(0) == 10);
    EXPECT_TRUE(tc2[0].key(0).use_count() == 1);
    EXPECT_TRUE(tc2[0].mapped(0) == 5);
    EXPECT_TRUE(tc2.get_allocator().id == 5);
}

TEST(bucket_container, swap) {
    allocator_wrapper<true, true, true>::stateful_allocator<value_type> a(5);
    TestingContainer<decltype(a)> tc(2, a);
    tc.setKV(0, 0, 2, std::make_shared<int>(10), 5);
    TestingContainer<decltype(a)> tc2(2, a);
    tc2.setKV(1, 0, 2, std::make_shared<int>(10), 5);

    tc.swap(tc2);

    EXPECT_TRUE(tc[1].occupied(0));
    EXPECT_TRUE(tc[1].partial(0) == 2);
    EXPECT_TRUE(*tc[1].key(0) == 10);
    EXPECT_TRUE(tc[1].key(0).use_count() == 1);
    EXPECT_TRUE(tc[1].mapped(0) == 5);
    EXPECT_TRUE(tc.get_allocator().id == 7);

    EXPECT_TRUE(tc2[0].occupied(0));
    EXPECT_TRUE(tc2[0].partial(0) == 2);
    EXPECT_TRUE(*tc2[0].key(0) == 10);
    EXPECT_TRUE(tc2[0].key(0).use_count() == 1);
    EXPECT_TRUE(tc2[0].mapped(0) == 5);
    EXPECT_TRUE(tc2.get_allocator().id == 7);
}

struct ExceptionInt {
    int x;
    static bool do_throw;

    ExceptionInt(int x_) : x(x_) { maybeThrow(); }

    ExceptionInt(const ExceptionInt &other) : x(other.x) { maybeThrow(); }

    ExceptionInt &operator=(const ExceptionInt &other) {
        x = other.x;
        maybeThrow();
        return *this;
    }

    ~ExceptionInt() { maybeThrow(); }

private:
    void maybeThrow() {
        if (do_throw) {
            throw std::runtime_error("thrown");
        }
    }
};

bool ExceptionInt::do_throw = false;

using ExceptionContainer =
abel::bucket_container<ExceptionInt, int,
        std::allocator<std::pair<ExceptionInt, int>>,
        uint8_t, SLOT_PER_BUCKET>;

TEST(bucket_container, set_kv) {
    ExceptionContainer container(0, ExceptionContainer::allocator_type());
    container.setKV(0, 0, 0, ExceptionInt(10), 20);

    ExceptionInt::do_throw = true;
    EXPECT_THROW(container.setKV(0, 1, 0, 0, 0), std::runtime_error);
    ExceptionInt::do_throw = false;

    EXPECT_TRUE(container[0].occupied(0));
    EXPECT_TRUE(container[0].key(0).x == 10);
    EXPECT_TRUE(container[0].mapped(0) == 20);

    EXPECT_FALSE(container[0].occupied(1));
}

TEST(bucket_container, move_assign) {
    ExceptionContainer container(0, ExceptionContainer::allocator_type());
    container.setKV(0, 0, 0, ExceptionInt(10), 20);
    ExceptionContainer other(0, ExceptionContainer::allocator_type());

    ExceptionInt::do_throw = true;
    EXPECT_THROW(other = container, std::runtime_error);
    ExceptionInt::do_throw = false;
}
