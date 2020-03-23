//
// Created by liyinbin on 2020/3/23.
//

#include <abel/asl/functional.h>
#include <abel/memory/memory.h>
#include <memory>
#include <gtest/gtest.h>


using namespace abel;

TEST(noncopyable_function, basic_tests) {
        struct s {
            int f1(int x) const { return x + 1; }
            int f2(int x) { return x + 2; }
            static int f3(int x) { return x + 3; }
            int operator()(int x) const { return x + 4; }
        };
        s obj, obj2;
        auto fn1 = noncopyable_function<int (const s*, int)>(&s::f1);
        auto fn2 = noncopyable_function<int (s*, int)>(&s::f2);
        auto fn3 = noncopyable_function<int (int)>(&s::f3);
        auto fn4 = noncopyable_function<int (int)>(std::move(obj2));
        EXPECT_EQ(fn1(&obj, 1), 2);
        EXPECT_EQ(fn2(&obj, 1), 3);
        EXPECT_EQ(fn3(1), 4);
        EXPECT_EQ(fn4(1), 5);
}

template <size_t Extra>
struct payload {
    static unsigned live;
    char extra[Extra];
    std::unique_ptr<int> v;
    payload(int x) : v(abel::make_unique<int>(x)) { ++live; }
    payload(payload&& x) noexcept : v(std::move(x.v)) { ++live; }
    void operator=(payload&&) = delete;
    ~payload() { --live; }
    int operator()() const { return *v; }
};

template <size_t Extra>
unsigned payload<Extra>::live;

template <size_t Extra>
void do_move_tests() {
    using payload = ::payload<Extra>;
    auto f1 = noncopyable_function<int ()>(payload(3));
    EXPECT_EQ(payload::live, 1u);
    EXPECT_EQ(f1(), 3);
    auto f2 = noncopyable_function<int ()>();
    EXPECT_THROW(f2(), std::bad_function_call);
    f2 = std::move(f1);
    EXPECT_THROW(f1(), std::bad_function_call);
    EXPECT_EQ(f2(), 3);
    EXPECT_EQ(payload::live, 1u);
    f2 = {};
    EXPECT_EQ(payload::live, 0u);
    EXPECT_THROW(f2(), std::bad_function_call);
}

TEST(noncopyable_function, small_move_tests) {
        do_move_tests<1>();
}

TEST(noncopyable_function, large_move_tests) {
        do_move_tests<1000>();
}


