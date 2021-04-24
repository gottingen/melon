//
// Created by liyinbin on 2021/4/3.
//


#include "abel/functional/function.h"

#include <array>
#include <vector>

#include "gtest/gtest.h"

namespace abel {

    int PlainOldFunction(int, double, char) { return 1; }

    TEST(function, Empty) { function<void()> f; }

    TEST(function, POF) {
        function<int(int, double, char)> f(PlainOldFunction);
        ASSERT_EQ(1, f(0, 0, 0));

#if __cpp_deduction_guides >= 201611L
        function f2(PlainOldFunction);  // Deduction guides come into play.
        ASSERT_EQ(1, f2(0, 0, 0));
#endif
    }

    TEST(function, Lambda) {
#if __cpp_deduction_guides >= 201611L
        function f([](int x) { return x; });
        ASSERT_EQ(12, f(12));
#endif

        function<int()> f2([] { return 1; });
        ASSERT_EQ(1, f2());
    }

    class FancyClass {
    public:
        int f(int x) { return x; }
    };

    TEST(function, MemberMethod) {
        function<int(FancyClass &, int)> f(&FancyClass::f);
        FancyClass fc;

        ASSERT_EQ(10, f(fc, 10));
    }

    TEST(function, LargeFunctorTest) {
        function<int()> f;
        std::array<char, 1000000> payload;

        payload.back() = 12;
        f = [payload] { return payload.back(); };
        ASSERT_EQ(12, f());
    }

    TEST(function, FunctorMoveTest) {
        struct OnlyCopyable {
            OnlyCopyable() : v(new std::vector<int>()) {}

            OnlyCopyable(const OnlyCopyable &oc) : v(new std::vector<int>(*oc.v)) {}

            ~OnlyCopyable() { delete v; }

            std::vector<int> *v;
        };
        function<int()> f, f2;
        OnlyCopyable payload;

        payload.v->resize(100, 12);

// BE SURE THAT THE LAMBDA IS NOT LARGER THAN kMaximumOptimizableSize.
        f = [payload] { return payload.v->back(); };
        f2 = std::move(f);
        ASSERT_EQ(12, f2());
    }

    TEST(function, LargeFunctorMoveTest) {
        function<int()> f, f2;
        std::array<std::vector<int>, 100> payload;

        payload.back().resize(10, 12);
        f = [payload] { return payload.back().back(); };
        f2 = std::move(f);
        ASSERT_EQ(12, f2());
    }

    TEST(function, CastAnyTypeToVoid) {
        function<void()> f;
        int x = 0;

        f = [&x]() -> int {
            x = 1;
            return x;
        };
        f();

        ASSERT_EQ(1, x);
    }

    TEST(function, Clear) {
        function<void()> f = [] {};

        ASSERT_TRUE(f);
        f = nullptr;
        ASSERT_FALSE(f);
    }

    TEST(scoped_deferred, All) {
        bool f = false;
        {
            scoped_deferred defer([&] { f = true; });
            ASSERT_FALSE(f);
        }
        ASSERT_TRUE(f);
    }

    TEST(Deferred, All) {
        bool f1 = false, f2 = false;
        {
            deferred defer([&] { f1 = true; });
            ASSERT_FALSE(f1);
            deferred defer2([&] { f2 = true; });
            defer2.dismiss();
            ASSERT_FALSE(f2);
        }
        ASSERT_TRUE(f1);
        ASSERT_FALSE(f2);
    }

}  // namespace abel

