// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com

#include "abel/atomic/atomic_hook.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "abel/base/profile.h"
#include "testing/atomic_hook_test_helper.h"

namespace {

    using ::testing::Eq;

    int value = 0;

    void TestHook(int x) { value = x; }

    TEST(AtomicHookTest, NoDefaultFunction) {
        ABEL_CONST_INIT static abel::atomic_hook<void (*)(int)> hook;
        value = 0;

        // Test the default DummyFunction.
        EXPECT_TRUE(hook.Load() == nullptr);
        EXPECT_EQ(value, 0);
        hook(1);
        EXPECT_EQ(value, 0);

        // Test a stored hook.
        hook.Store(TestHook);
        EXPECT_TRUE(hook.Load() == TestHook);
        EXPECT_EQ(value, 0);
        hook(1);
        EXPECT_EQ(value, 1);

        // Calling Store() with the same hook should not crash.
        hook.Store(TestHook);
        EXPECT_TRUE(hook.Load() == TestHook);
        EXPECT_EQ(value, 1);
        hook(2);
        EXPECT_EQ(value, 2);
    }

    TEST(AtomicHookTest, WithDefaultFunction) {
        // Set the default value to TestHook at compile-time.
        ABEL_CONST_INIT static abel::atomic_hook<void (*)(int)> hook(
                TestHook);
        value = 0;

        // Test the default value is TestHook.
        EXPECT_TRUE(hook.Load() == TestHook);
        EXPECT_EQ(value, 0);
        hook(1);
        EXPECT_EQ(value, 1);

        // Calling Store() with the same hook should not crash.
        hook.Store(TestHook);
        EXPECT_TRUE(hook.Load() == TestHook);
        EXPECT_EQ(value, 1);
        hook(2);
        EXPECT_EQ(value, 2);
    }

    ABEL_CONST_INIT int override_func_calls = 0;

    void OverrideFunc() { override_func_calls++; }

    static struct OverrideInstaller {
        OverrideInstaller() { abel::atomic_hook_internal::func.Store(OverrideFunc); }
    } override_installer;

    TEST(AtomicHookTest, DynamicInitFromAnotherTU) {
        // MSVC 14.2 doesn't do constexpr static init correctly; in particular it
        // tends to sequence static init (i.e. defaults) of `atomic_hook` objects
        // after their dynamic init (i.e. overrides), overwriting whatever value was
        // written during dynamic init.  This regression test validates the fix.
        // https://developercommunity.visualstudio.com/content/problem/336946/class-with-constexpr-constructor-not-using-static.html
        EXPECT_THAT(abel::atomic_hook_internal::default_func_calls, Eq(0));
        EXPECT_THAT(override_func_calls, Eq(0));
        abel::atomic_hook_internal::func();
        EXPECT_THAT(abel::atomic_hook_internal::default_func_calls, Eq(0));
        EXPECT_THAT(override_func_calls, Eq(1));
        EXPECT_THAT(abel::atomic_hook_internal::func.Load(), Eq(OverrideFunc));
    }

}  // namespace
