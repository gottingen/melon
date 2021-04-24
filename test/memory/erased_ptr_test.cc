//
// Created by liyinbin on 2021/4/3.
//

#include "abel/memory/erased_ptr.h"

#include "gtest/gtest.h"

namespace abel {

    struct C {
        C() { ++instances; }

        ~C() { --instances; }

        inline static int instances = 0;
    };

    TEST(erased_ptr, All) {
        ASSERT_EQ(0, C::instances);
        {
            erased_ptr ptr(new C{});
            ASSERT_EQ(1, C::instances);
            auto deleter = ptr.get_deleter();
            auto p = ptr.leak();
            ASSERT_EQ(1, C::instances);
            deleter(p);
            ASSERT_EQ(0, C::instances);
        }
        ASSERT_EQ(0, C::instances);
        {
            erased_ptr ptr(new C{});
            ASSERT_EQ(1, C::instances);
        }
        ASSERT_EQ(0, C::instances);
    }

}  // namespace abel