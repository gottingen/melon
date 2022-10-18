
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/


#include <iostream>
#include <queue>
#include "testing/gtest_wrap.h"
#include "melon/future/future.h"
#include <array>
#include <queue>


template<typename T>
struct Test_alloc {
    using value_type = T;

    template<typename U>
    explicit Test_alloc(const Test_alloc<U> &) {}

    template<typename U>
    struct rebind {
        using other = Test_alloc<U>;
    };

    T *allocate(std::size_t count) {
        return reinterpret_cast<T *>(std::malloc(count * sizeof(T)));
    }

    void deallocate(T *ptr, std::size_t) { std::free(ptr); }
};

template<>
struct Test_alloc<void> {
    using value_type = void;

    template<typename U>
    struct rebind {
        using other = Test_alloc<U>;
    };
};

TEST(future_join, simple_join) {
    melon::promise<int> p1;
    melon::promise<int> p2;

    auto f = join(p1.get_future(), p2.get_future()).then([](int x, int y) {
        return x + y;
    });

    std::cout << "aa\n";
    p1.set_value(1);
    p2.set_value(2);

    std::cout << "bb\n";
    ASSERT_EQ(3, f.get());
}

TEST(future_join, simple_join_with_allocator) {
    melon::basic_promise<Test_alloc<void>, int> p1;
    melon::basic_promise<Test_alloc<void>, int> p2;

    auto f = join(p1.get_future(), p2.get_future()).then([](int x, int y) {
        return x + y;
    });

    p1.set_value(1);
    p2.set_value(2);

    ASSERT_EQ(3, f.get());
}
