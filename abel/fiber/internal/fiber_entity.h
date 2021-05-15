//
// Created by liyinbin on 2021/4/4.
//

#ifndef ABEL_FIBER_INTERRNAL_FIBER_ENTRITY_H_
#define ABEL_FIBER_INTERRNAL_FIBER_ENTRITY_H_

#include <unistd.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <unordered_map>

#include "abel/base/annotation.h"
#include "abel/functional/function.h"
#include "abel/fiber/internal/spin_lock.h"
#include "abel/log/logging.h"
#include "abel/memory/erased_ptr.h"
#include "abel/memory/ref_ptr.h"
#include "abel/fiber/internal/context.h"



namespace abel {

    namespace fiber_internal {

        class exit_barrier;

        class scheduling_group;

        enum class fiber_state {
            Ready, Running, Waiting, Dead
        };

        // Space reserved at stack bottom for `fiber_entity`.
        constexpr auto kFiberStackReservedSize = 512;

        // @sa: `fiber_entity::ever_started_magic`.
        constexpr std::uint64_t kFiberEverStartedMagic = 0x11223344ABABBBAA;

        // This structure is stored at the top of fiber's stack. (i.e., highest
        // address). Everything related to the fiber (control structure) are defined
        // here.
        //
        // Note that `fiber_entity` is stored starting at 64-byte (@sa: kFiberMagicSize)
        // offset in last stack page. The beginning `kFiberMagicSize` bytes are used to
        // store magic number.
        struct fiber_entity {

            using trivial_fls_t = std::aligned_storage_t<8, 8>;

            // This constant determines how many FLS (fiber local storage) are stored in
            // place (inside `fiber_entity`). This helps improving performance in exchange
            // of memory footprint.
            //
            // I haven't test if it suits our need, it's arbitrarily chosen.
            static constexpr auto kInlineLocalStorageSlots = 8;

            // We optimize for FLS of primitive types by storing them inline (without
            // incurring overhead of `erased_ptr`). This constant controls how many slots
            // are reserved (separately from non-primitive types) for them.
            //
            //   For the moment we only uses trivial FLS in our framework.
            //
            // Only types no larger than `trivial_fls_t` can be stored this way.
            static constexpr auto kInlineTrivialLocalStorageSlots = 8;

            // We don't initialize primitive fields below, they're filled by
            // `create_fiber_entity`. Surprisingly GCC does not eliminate these dead stores
            // (as they're immediately overwritten by `create_fiber_entity`) if we do
            // initialize them here.

            // The first `kFiberMagicSize` bytes serve as a magic for identifying fiber's
            // stack (as well as it's control structure, i.e., us). The magic is:
            // "ABEL_FIBER_ENTITY"
            //
            // It's filled by `create_user_stack` when the stack is created, we don't want
            // to initialize it each time a fiber is created.
            //
            // char magic[kFiberMagicSize];

            // Fiber ID for `gdb-plugin.py` to use.
            std::uint64_t debugging_fiber_id;

            // Set the first time internal fiber start callback is run. This is used
            // primarily by our GDB plugin to ignore never-started fibers. Stack of those
            // fibers are in a mess, so we don't want to dump them out.
            //
            // We're using a magic number instead of a simple boolean to improve
            // robustness of detecting alive fibers. Once the `fiber_entity` is gone, value
            // here is not guaranteed by the standard. By using a magic number here we
            // reduce the possibility that a dead fiber is recognized as running
            // mistakenly.
            //
            // Marked as `volatile` to prevent compiler optimization.
            volatile std::uint64_t ever_started_magic;

            // This lock is held when the fiber is in state-transition (e.g., from running
            // to suspended). This is required since it's inherent racy when we add
            // ourselves into some wait-chain (and eventually woken up by someone else)
            // and go to sleep. The one who wake us up can be running in a different
            // pthread, and therefore might wake us up even before we actually went sleep.
            // So we always grab this lock before transiting the fiber's state, to ensure
            // that nobody else can change the fiber's state concurrently.
            //
            // For waking up a fiber, this lock is grabbed by whoever the waker;
            // For a fiber to go to sleep, this lock is grabbed by the fiber itself and
            // released by *`scheduling_group`* (by the time we're sleeping, we cannot
            // release the lock ourselves.).
            //
            // This lock also protects us from being woken up by several pthread
            // concurrently (in case we waited on several waitables and have not removed
            // us from all of them before more than one of then has fired.).
            abel::fiber_internal::spinlock scheduler_lock;

            // Fiber's affinity to this scheduling group.
            //
            // Set if the fiber should not be stolen to workers that does not belong to
            // the scheduling group specified on fiber's creation.
            bool scheduling_group_local;

            // Set if this fiber was created as a system fiber.
            //
            // System fiber uses a smaller stack, and don't use guard page to detect stack
            // overflow.
            bool system_fiber;

            // Fiber's state.
            fiber_state state = fiber_state::Ready;

            // Set by `scheduling_group::ready_fiber`.
            //
            // `waitable`s always schedule fibers that should be woken to the scheduling
            // group specified here.
            scheduling_group *own_scheduling_group;

            // When swapped out, fiber's context is saved here (top of the stack).
            void *state_save_area;


            // Updated when fiber is ready.
            abel::time_point last_ready_tsc;

            // Set if there is a pending `ResumeOn`. Cleared once `ResumeOn` completes.
            abel::function<void()> resume_proc;

            // Stack limit. 0 for master fiber.
            std::size_t stack_size;

            // This latch allows you to wait for this fiber's exit. It's needed for
            // implementing `Fiber::join()`.
            //
            // Because we have no idea about which one (`Fiber` or us) will be destroyed
            // first, we share it between `Fiber` and us.
            //
            // Note that for perf. reasons, this one need to be filled by the fiber
            // creator (out of `create_fiber_entity`) before adding the fiber into run
            // queue, if the caller indeed wants to join on the fiber.
            ref_ptr<exit_barrier> ref_exit_barrier;

            // Fiber local variables stored inline.
            erased_ptr inline_fls[kInlineLocalStorageSlots];

            // Fiber local variables of primitive types stored inline.
            trivial_fls_t inline_trivial_fls[kInlineTrivialLocalStorageSlots] = {};

            // In case `inline_fls` is not sufficient for storing FLS, `external_fls` is
            // used.
            //
            // However, accessing FLS-es in `external_fls` can be a magnitude slower than
            // `inline_fls`.
            std::unique_ptr<std::unordered_map<std::size_t, erased_ptr>> external_fls;
            std::unique_ptr<std::unordered_map<std::size_t, trivial_fls_t>> external_trivial_fls;

            // Entry point of this fiber. Cleared on first time the fiber is run.
            abel::function<void()> start_proc;

#ifdef ABEL_INTERNAL_USE_ASAN
            // Lowest address of this fiber's stack.
  const void* asan_stack_bottom = nullptr;

  // Stack limit of this fiber.
  std::size_t asan_stack_size = 0;

  // Set when the fiber is exiting. Special care must be taken in this case
  // (@sa: `SanitizerStartSwitchFiber`).
  bool asan_terminating = false;
#endif  // ABEL_INTERNAL_USE_ASAN

#ifdef ABEL_INTERNAL_USE_TSAN
            // Fiber context used by TSan.
  void* tsan_fiber = nullptr;
#endif  // ABEL_INTERNAL_USE_TSAN

            // Get top (highest address) of the runtime stack (after skipping this
            // control structure).
            //
            // Calling this method on main fiber is undefined.
            void *get_stack_top() const noexcept {
                // The runtime stack is placed right below us.
                return reinterpret_cast<char *>(const_cast<fiber_entity *>(this));
            }

            // Get stack size.
            std::size_t get_stack_limit() const noexcept { return stack_size; }

            // Switch to this fiber.
            void resume() noexcept;

            // Run code on top this fiber's context (or, if you're experienced with
            // Windows kernel, it's Asynchronous Procedure Call), and then resume this
            // fiber.
            void resume_on(abel::function<void()> &&cb) noexcept;

            // FLS are stored separately for trivial and non-trivial case. Note that you
            // don't have to (and shouldn't) use same index sequence for the two case.

            // Get FLS by its index.
            erased_ptr *get_fls(std::size_t index) noexcept {
                if (ABEL_LIKELY(index < std::size(inline_fls))) {
                    return &inline_fls[index];
                }
                return get_fls_slow(index);
            }

            erased_ptr *get_fls_slow(std::size_t index) noexcept;

            // Get trivial FLS by its index.
            //
            // Trivial FLSes are always zero-initialized.
            trivial_fls_t *get_trivial_fls(std::size_t index) noexcept {
                if (ABEL_LIKELY(index < std::size(inline_trivial_fls))) {
                    return &inline_trivial_fls[index];
                }
                return get_trivial_fls_slow(index);
            }

            trivial_fls_t *get_trivial_fls_slow(std::size_t index) noexcept;
        };

        static_assert(sizeof(fiber_entity) < kFiberStackReservedSize);

        // The linker should be able to relax these TLS to local-exec when linking
        // executables even if we don't specify it explicitly, but it doesn't (in my
        // test it only relaxed them to initial-exec). However, specifying initial-exec
        // explicitly results in local-exec being used. Thus we specify initial-exec
        // here. (Do NOT use local-exec though, it breaks UT, since we're linking UT
        // dynamically.).
        //
        // We use pointer here to avoid call to `__tls_init()` each time it's accessed.
        // The "real" master fiber object is defined inside `set_up_master_fiber_entity()`.
        //
        // By defining these TLS as `inline` here, compiler can be sure they need no
        // special initialization (as the compiler can see their definition, not just
        // declaration). Had we defined them in `fiber_entity.cc`, call to "TLS init
        // function for ..." might be needed by `GetXxxFiberEntity()` below.
        ABEL_INTERNAL_TLS_MODEL inline thread_local fiber_entity *master_fiber;
        ABEL_INTERNAL_TLS_MODEL inline thread_local fiber_entity *current_fiber;

        // Set up & get master fiber (i.e., so called "main" fiber) of this thread.
        void set_up_master_fiber_entity() noexcept;

#if defined(ABEL_INTERNAL_USE_TSAN) || defined(__powerpc64__) || \
    defined(__aarch64__)

        fiber_entity* get_master_fiber_entity() noexcept;
        fiber_entity* get_current_fiber_entity() noexcept;
        void set_current_fiber_entity(fiber_entity* current);

#else

        // For the moment we haven't seen any issue with this fast implementation when
        // using GCC8.2 on x86-64, so we keep them here for better perf.

        // Get & set fiber entity associated with current fiber.
        //
        // The set method is FOR INTERNAL USE ONLY.
        inline fiber_entity *get_current_fiber_entity() noexcept { return current_fiber; }

        inline void set_current_fiber_entity(fiber_entity *current) noexcept {
            current_fiber = current;
        }

        inline fiber_entity *get_master_fiber_entity() noexcept { return master_fiber; }

#endif

        // Mostly used for debugging purpose.
        inline bool is_fiber_context_present() noexcept {
            return get_current_fiber_entity() != nullptr;
        }

        // Create & destroy fiber entity (both the control structure and the stack.)
        fiber_entity *create_fiber_entity(scheduling_group *sg, bool system_fiber,
                                          abel::function<void()> &&start_proc) noexcept;

        void free_fiber_entity(fiber_entity *fiber) noexcept;


        template<class F>
        inline void destructive_run_callback(F *cb) {
            (*cb)();
            *cb = nullptr;
        }

        template<class F>
        inline void destructive_run_callback_opt(F *cb) {
            if (*cb) {
                (*cb)();
                *cb = nullptr;
            }
        }

        // This method is used so often that it deserves to be inlined.
        inline void fiber_entity::resume() noexcept {
            // Note that there are some inconsistencies. The stack we're running on is not
            // our stack. This should be easy to see, since we're actually running in
            // caller's context (including its stack).
            auto caller = get_current_fiber_entity();
            DCHECK(caller != this, "Calling `resume()` on self is undefined.");

#ifdef ABEL_INTERNAL_USE_ASAN
            // Here we're running on our caller's stack, not ours (the one associated with
      // `*this`.).
      void* shadow_stack;

      // Special care must be taken if the caller is being terminated. In this case,
      // the shadow stack associated with caller must be destroyed. We accomplish
      // this by passing `nullptr` to the call (@sa: `asan::start_switch_fiber`.).
      abel::asan::start_switch_fiber(
          caller->asan_terminating ? nullptr : &shadow_stack,  // Caller's shadow
                                                               // stack.
          asan_stack_bottom,  // The stack being swapped in.
          asan_stack_size);   // The stack being swapped in.
#endif

#ifdef ABEL_INTERNAL_USE_TSAN
            abel::tsan::switch_to_fiber(tsan_fiber);
#endif

            // Argument `context` (i.e., `this`) is only used the first time the context
            // is jumped to (in `FiberProc`).
            fiber_jump_context(&caller->state_save_area, state_save_area, reinterpret_cast<intptr_t>(this));
            //abel_fiber_swap(&caller->context, &context);

#ifdef ABEL_INTERNAL_USE_ASAN
            DCHECK(!caller->asan_terminating);  // Otherwise the caller (as well as
                                               // this runtime stack) has gone.

      // We're back to the caller's runtime stack. Restore its shadow stack.
      abel::asan::complete_switch_fiber(shadow_stack);
#endif

            set_current_fiber_entity(caller);  // The caller has back.

            // Check for pending `ResumeOn`.
            destructive_run_callback_opt(&caller->resume_proc);
        }

    }  // namespace fiber_internal

}  // namespace abel

#endif  // ABEL_FIBER_INTERRNAL_FIBER_ENTRITY_H_
