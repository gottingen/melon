//
// Created by liyinbin on 2021/4/2.
//

#include "gtest/gtest.h"
#include "abel/memory/non_destroy.h"

namespace abel {
    struct C {
        C() { ++instances; }

        ~C() { --instances; }

        inline static std::size_t instances{};
    };

    struct D {
        void foo() {
            [[maybe_unused]] static non_destroyed_singleton <D> test_compilation2;
        }
    };

    non_destroy<int> test_compilation2;

    TEST(non_destroy, All) {
        ASSERT_EQ(0, C::instances);
        {
            C c1;
            ASSERT_EQ(1, C::instances);
            [[maybe_unused]] non_destroy<C> c2;
            ASSERT_EQ(2, C::instances);
        }
// Not 0, as `non_destroy<C>` is not destroyed.
        ASSERT_EQ(1, C::instances);
    }

}