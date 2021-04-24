//
// Created by liyinbin on 2021/4/3.
//


#include "abel/memory/object_pool.h"

#include <thread>
#include <vector>

#include "gtest/gtest.h"

using namespace std::literals;

namespace abel {

    int alive = 0;

    struct C {
        C() { ++alive; }

        ~C() { --alive; }
    };

    template<>
    struct pool_traits<C> {
        static constexpr auto kType = pool_type::ThreadLocal;
        static constexpr auto kLowWaterMark = 16;
        static constexpr auto kHighWaterMark = 128;
        static constexpr auto kMaxIdle = abel::duration::milliseconds(3000);
    };

    struct D {
        int x;
        inline static int put_called = 0;
    };

    template<>
    struct pool_traits<D> {
        static constexpr auto kType = pool_type::ThreadLocal;
        static constexpr auto kLowWaterMark = 16;
        static constexpr auto kHighWaterMark = 128;
        static constexpr auto kMaxIdle = abel::duration::milliseconds(3000);

        static void OnGet(D *p) { p->x = 0; }

        static void OnPut(D *) { ++D::put_called; }
    };

    namespace object_pool {

        TEST(ThreadLocalPool, All) {
            std::vector<pooled_ptr<C>> ptrs;
            for (int i = 0; i != 1000; ++i) {
                ptrs.push_back(get<C>());
            }
            ptrs.clear();
            for (int i = 0; i != 200; ++i) {
                std::this_thread::sleep_for(10ms);
                get<C>().reset();  // Trigger wash out if possible.
            }
            ASSERT_EQ(pool_traits<C>::kHighWaterMark, alive);  // High-water mark.

            // Max idle not reached. No effect.
            get<C>().reset();
            ASSERT_EQ(pool_traits<C>::kHighWaterMark, alive);

            std::this_thread::sleep_for(5000ms);
            for (int i = 0; i != 100; ++i) {
                get<C>().reset();  // We have a limit on objects to loop per call. So we may
            // need several calls to lower alive objects to low-water
            // mark.
                std::this_thread::sleep_for(10ms);  // The limit on wash interval..
            }
            // Low-water mark. + 1 for the object just freed (as it's fresh and won't be
            // affected by low-water mark.).
            ASSERT_EQ(pool_traits<C>::kLowWaterMark + 1, alive);

            put<C>(get<C>().leak());  // Don't leak.
            ASSERT_EQ(pool_traits<C>::kLowWaterMark + 1, alive);
        }

        TEST(ThreadLocalPool, OnGetHook) {
            { auto ptr = get<D>(); }
            {
                auto ptr = get<D>();
                ASSERT_EQ(0, ptr->x);
            }
            ASSERT_EQ(2, D::put_called);
        }

    }  // namespace object_pool
}  // namespace abel
