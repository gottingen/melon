//
// Created by liyinbin on 2021/4/5.
//


#include "abel/fiber/internal/stack_allocator.h"

#include <unistd.h>
#include "gtest/gtest.h"
#include "gflags/gflags.h"
#include "abel/base/annotation.h"
#include "abel/log/logging.h"


DECLARE_int32(fiber_stack_size);

namespace abel {
    namespace fiber_internal {

// If ASan is present, the death test below triggers ASan and aborts.
#ifndef ABEL_INTERNAL_USE_ASAN

        TEST(StackAllocatorDeathTest, SystemStackCanaryValue
        ) {
            auto stack = create_system_stack();
            ASSERT_TRUE(stack);
            memset(stack,
                   0, 8192);  // Canary value is overwritten.
            ASSERT_DEATH(free_system_stack(stack),
                         "stack is corrupted");
        }

#endif

        TEST(StackAllocator, user_stack
        ) {
            auto stack = create_user_stack();
            ASSERT_TRUE(stack);
            memset(stack,
                   0, FLAGS_fiber_stack_size);
            free_user_stack(stack);
        }

#ifndef ABEL_INTERNAL_USE_ASAN

        TEST(StackAllocator, system_stack
        ) {
            auto stack = create_system_stack();
            ASSERT_TRUE(stack);
            memset(reinterpret_cast<char *>(stack)+ 16 /* Canary value */, 0,kSystemStackSize - 16);
            free_system_stack(stack);
        }

#else

        TEST(StackAllocator, system_stack) {
          auto stack = create_system_stack();
          ASSERT_TRUE(stack);
          memset(reinterpret_cast<char*>(stack) +
                     kSystemStackPoisonedSize /* Poisoned page */,
                 0, kSystemStackSize - kSystemStackPoisonedSize.get());
          free_system_stack(stack);
        }

#endif

    }  // namespace fiber_internal
}  // namespace abel
