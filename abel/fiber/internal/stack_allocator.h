//
// Created by liyinbin on 2021/4/4.
//

#ifndef ABEL_FIBER_INTERNAL_STACK_ALLOCATOR_H_
#define ABEL_FIBER_INTERNAL_STACK_ALLOCATOR_H_

#include <chrono>
#include <limits>
#include <utility>

#include "gflags/gflags.h"
#include "abel/memory/object_pool.h"
#include "abel/log/logging.h"

DECLARE_int32(fiber_stack_size);

namespace abel {
namespace fiber_internal {
    // Here we define two types of stacks:
    //
    // - User stack: Where user's code will be running on. Its size can be
    //   controlled by `FLAGS_fiber_stack_size`. It's also possible to enable
    //   guard page (as by default) for this type of stacks.
    //
    //   However, creating such a stack does incur some overhead:
    //
    //   - To keep memory footprint low and not reach system limit on maximum number
    //     of VMAs. This type of stack is not cached a lot per thread.
    //
    //   - Large stack size uses memory, as obvious.
    //
    //   - Allocating guard page requires a dedicated VMA.
    //
    // - System stack: This type of stack is used solely by the framework. It's size
    //   is statically determined (@sa: `kSystemStackSize`), and no guard page is
    //   provided.
    //
    //   Obviously, using stack of such type is more dangerous than "user stack".
    //
    //   However, it does brings us some good, notably the elimination of overhead
    //   of user stack.

    // Merely tagging types. They're used for specializing `pool_traits<...>`.
    //
    // The real "object" represented by it is a fiber stack (contiguous pages
    // allocated by `create_user_stack_impl()`.).
        struct user_stack {};
        struct system_stack {};

        user_stack* create_user_stack_impl();
        void destroy_user_stack_impl(user_stack* ptr);

        system_stack* create_system_stack_impl();
        void destroy_system_stack_impl(system_stack* ptr);

// System stacks are used solely by us. It's our own responsibility not to
// overflow the stack. This restriction permits several optimization
// possibilities.
#ifdef ABEL_INTERNAL_USE_ASAN
        constexpr auto kSystemStackPoisonedSize = 4096;
constexpr auto kSystemStackSize = 16384 + kSystemStackPoisonedSize;
#else
        constexpr auto kSystemStackSize = 16384 * 2;
#endif
}  // namespace fiber_internal
}  // namespace abel

namespace abel {

    template <>
    struct pool_traits<abel::fiber_internal::user_stack> {
        using user_stack = abel::fiber_internal::user_stack;

        static constexpr auto kType = pool_type::ThreadLocal;
        static constexpr auto kLowWaterMark = 512;
        // Don't set high water-mark too large, or we risk running out of
        // `vm.max_map_count`.
        static constexpr auto kHighWaterMark = 16384;
        static constexpr auto kMaxIdle = abel::duration::seconds(10);
        static constexpr auto kMinimumThreadCacheSize = 32;
        // If we allocate more stack than necessary, we're risking reaching
        // max_map_count limit.
        static constexpr auto kTransferBatchSize = 128;

        static auto create() {
            auto ptr = abel::fiber_internal::create_user_stack_impl();

#ifdef ABEL_INTERNAL_USE_ASAN
            // Poisoned immediately. It's un-poisoned prior to use.
    abel::asan::poison_memory_region(ptr,
                                              FLAGS_fiber_stack_size);
#endif

            return ptr;
        }

        static void destroy(fiber_internal::user_stack* ptr) {
#ifdef ABEL_INTERNAL_USE_ASAN
            // Un-poisoned prior to free so as not to interference with other
    // allocations.
    abel::asan::UnpoisonMemoryRegion(ptr,
                                                FLAGS_fiber_stack_size);
#endif

            destroy_user_stack_impl(ptr);
        }

        // Canary value here? We've done this for system stack, where guard page is
        // not applicable.

#ifdef ABEL_INTERNAL_USE_ASAN

        // The stack is not "unpoison"-ed (if ASAN is in use) on get and re-poisoned
  // on put. This helps us to detect use-after-free of the stack as well.
  static void OnGet(user_stack* ptr) {
    abel::asan::UnpoisonMemoryRegion(ptr, FLAGS_fiber_stack_size.get());
  }

  static void OnPut(user_stack* ptr) {
    abel::asan::poison_memory_region(ptr,
                                              FLAGS_fiber_stack_size.get());
  }

#endif
    };

    template <>
    struct pool_traits<abel::fiber_internal::system_stack> {
        // There's no guard page for system stack. However, we still want to detect
        // stack overflow (in a limited fashion). Therefore, we put these canary
        // values at stack limit. If they got overwritten, stack overflow can be
        // detected at stack deallocation (in `OnPut`).
        //
        // EncodeHex("AbelStackCanary"): 466c617265537461636b43616e617279
        inline static constexpr std::uint64_t kStackCanary0 = 0x466c'6172'6553'7461;
        inline static constexpr std::uint64_t kStackCanary1 = 0x636b'4361'6e61'7279;

        using system_stack = abel::fiber_internal::system_stack;

        static constexpr auto kType = pool_type::ThreadLocal;
        static constexpr auto kLowWaterMark = 4096;
        static constexpr auto kHighWaterMark =
                std::numeric_limits<std::size_t>::max();
        static constexpr auto kMaxIdle = abel::duration::seconds(10);
        static constexpr auto kMinimumThreadCacheSize = 128;
        static constexpr auto kTransferBatchSize = 512;

        static auto create() {
            auto ptr =abel::fiber_internal::create_system_stack_impl();

#ifndef ABEL_INTERNAL_USE_ASAN
            // Canary value is not of much use if ASan is enabled. In case of ASan, we
            // poison the bytes at stack limit. This is more powerful than checking
            // canary values on stack deallocation.
            initialize_canary_value(ptr);
#endif

#ifdef ABEL_INTERNAL_USE_ASAN
            // The memory region is poisoned immediately. We'll un-poison it prior to
    // use.
    abel::asan::poison_memory_region(ptr,
                                              abel::fiber_internal::kSystemStackSize);
#endif

            return ptr;
        }

        static void destroy(system_stack* ptr) {
#ifdef ABEL_INTERNAL_USE_ASAN
            // Un-poison these bytes before returning them to the runtime.
    abel::asan::UnpoisonMemoryRegion(
        ptr, abel::fiber_internal::kSystemStackSize);
#endif

            destroy_system_stack_impl(ptr);
        }

        static void OnGet(system_stack* ptr) {
#ifndef ABEL_INTERNAL_USE_ASAN
            // Make sure our canary is still there. We only do this check if ASan is NOT
            // enabled. If ASan is enabled, these bytes are poisoned and any access to
            // then should have already trigger an error report.
            verify_canary_value(ptr);
#endif

#ifdef ABEL_INTERNAL_USE_ASAN
            // The first bytes are left poisoned. They acts as the same role as "guard
    // page".
    auto&& [usable, limit] = SplitMemoryRegionForStack(ptr);
    abel::asan::UnpoisonMemoryRegion(usable, limit);
#endif
        }

        static void OnPut(system_stack* ptr) {
#ifndef ABEL_INTERNAL_USE_ASAN
            // Don't overflow our stack.
            verify_canary_value(ptr);
#endif

#ifdef ABEL_INTERNAL_USE_ASAN
    // Re-poisoned so that we can detect use-after-free on stack.
    auto&& [usable, limit] = SplitMemoryRegionForStack(ptr);
    abel::asan::poison_memory_region(usable, limit);
#endif
        }

        // `ptr` points to the first byte (i.e., stack limit) of the stack.
        static void initialize_canary_value(system_stack* ptr) {
            static_assert(sizeof(std::uint64_t) == 8);

            auto canaries = reinterpret_cast<volatile std::uint64_t*>(ptr);
            canaries[0] = kStackCanary0;
            canaries[1] = kStackCanary1;
        }

        static void verify_canary_value(system_stack* ptr) {
            auto canaries = reinterpret_cast<std::uint64_t*>(ptr);  // U.B.?
            DCHECK_MSG(
                    kStackCanary0 == canaries[0],
                    "The first canary value was overwritten. The stack is corrupted.");
            DCHECK_MSG(
                    kStackCanary1 == canaries[1],
                    "The second canary value was overwritten. The stack is corrupted.");
        }

#ifdef ABEL_INTERNAL_USE_ASAN
        static std::pair<void*, std::size_t> SplitMemoryRegionForStack(void* ptr) {
    return {
        reinterpret_cast<char*>(ptr) + abel::fiber_internal::kSystemStackPoisonedSize,
        abel::fiber_internal::kSystemStackSize -
            abel::fiber_internal::kSystemStackPoisonedSize};
  }
#endif
    };

}  // namespace abel

namespace abel {
    namespace fiber_internal {

        // Allocate a memory block of size `FLAGS_fiber_stack_size`.
        //
        // The result points to the top of the stack (lowest address).
        inline void* create_user_stack() { return object_pool::get<abel::fiber_internal::user_stack>().leak(); }

        // `stack` should point to the top of the stack.
        inline void free_user_stack(void* stack) {
            object_pool::put<abel::fiber_internal::user_stack>(reinterpret_cast<abel::fiber_internal::user_stack*>(stack));
        }

        // For allocating / deallocating system stacks.
        inline void* create_system_stack() {
            return object_pool::get<abel::fiber_internal::system_stack>().leak();
        }

        inline void free_system_stack(void* stack) {
            object_pool::put<abel::fiber_internal::system_stack>(reinterpret_cast<abel::fiber_internal::system_stack*>(stack));
        }

    }  // namespace fiber_internal
}  // namespace abel
#endif  // ABEL_FIBER_INTERNAL_STACK_ALLOCATOR_H_
