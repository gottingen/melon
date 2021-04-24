//
// Created by liyinbin on 2021/4/4.
//


#include "abel/fiber/internal/fiber_entity.h"
#include "abel/fiber/internal/id_alloc.h"
#include "abel/fiber/internal/scheduling_group.h"

#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1  // `pthread_getattr_np` needs this.
#endif

#include <pthread.h>

#include <limits>
#include <utility>

#include "abel/functional/function.h"
#include "abel/base/annotation.h"
#include "abel/log//logging.h"
#include "abel/fiber/internal/scheduling_group.h"
#include "abel/fiber/internal/stack_allocator.h"
#include "abel/fiber/internal/waitable.h"
#include "gflags/gflags.h"


DECLARE_int32(fiber_stack_size);


namespace abel {
    namespace fiber_internal {
        namespace {

            struct fiber_id_traits {
                using Type = std::uint64_t;
                static constexpr auto kMin = 1;
                static constexpr auto kMax = std::numeric_limits<std::uint64_t>::max();
                // I don't expect a pthread worker need to create more than 128K fibers per
                // sec.
                static constexpr auto kBatchSize = 131072;
            };

#ifdef ABEL_INTERNAL_USE_ASAN

    // Returns: stack bottom (lowest address) and limit of current pthread (i.e.
    // master fiber's stack).
    std::pair<const void*, std::size_t> get_master_fiber_stack() noexcept {
      pthread_attr_t self_attr;
      DCHECK(pthread_getattr_np(pthread_self(), &self_attr) == 0);
      scoped_deferred _(
          [&] { DCHECK(pthread_attr_destroy(&self_attr) == 0); });

      void* stack;
      std::size_t limit;
      DCHECK(pthread_attr_getstack(&self_attr, &stack, &limit) == 0);
      return {stack, limit};
    }

#endif

        }  // namespace

        // Entry point for newly-started fibers.
        //
        // NOT put into anonymous namespace to simplify its displayed name in GDB.
        //
        // `extern "C"` doesn't help, unfortunately. (It does simplify mangled name,
        // though.)
        //
        // Do NOT mark this function as `noexcept`. We don't want to force stack being
        // unwound on exception.
        static void fiber_proc(void *context) {
            auto self = reinterpret_cast<fiber_entity *>(context);
            // We're running in `self`'s stack now.

#ifdef ABEL_INTERNAL_USE_ASAN
            // A new fiber has born, so complete with a new shadow stack.
      //
      // By passing `nullptr` to this call, a new shadow stack is allocated
      // internally. (@sa: `asan::CompleteSwitchFiber`).
      abel::asan::complete_switch_fiber(nullptr);
#endif

            set_current_fiber_entity(self);  // We're alive.
            self->state = fiber_state::Running;
            self->ever_started_magic = kFiberEverStartedMagic;

            // Hmmm, there is a pending resumption callback, even if we haven't completely
            // started..
            //
            // We'll run it anyway. This, for now, is mostly used for `Dispatch` fiber
            // launch policy.
            destructive_run_callback_opt(&self->resume_proc);
            destructive_run_callback(&self->start_proc);

            // We're leaving now.
            DCHECK_EQ(self, get_current_fiber_entity());

            // This fiber should not be waiting on anything (mutex / condition_variable
            // / ...), i.e., no one else should be referring this fiber (referring to its
            // `exit_barrier` is, since it's ref-counted, no problem), otherwise it's a
            // programming mistake.

            // Let's see if there will be someone who will be waiting on us.
            if (!self->ref_exit_barrier) {
                // Mark the fiber as dead. This prevent our GDB plugin from listing this
                // fiber out.
                self->state = fiber_state::Dead;

#ifdef ABEL_INTERNAL_USE_ASAN
                // We're leaving, a special call to asan is required. So take special note
        // of it.
        //
        // Consumed by `fiber_entity::resume()` prior to switching stack.
        self->asan_terminating = true;
#endif

                // No one is waiting for us, this is easy.
                get_master_fiber_entity()->resume_on([self] { free_fiber_entity(self); });
            } else {
                // The lock must be taken first, we can't afford to block when we (the
                // callback passed to `ResumeOn()`) run on master fiber.
                //
                // CAUTION: WE CAN TRIGGER RESCHEDULING HERE.
                auto ebl = self->ref_exit_barrier->GrabLock();

                // Must be done after `GrabLock()`, as `GrabLock()` is self may trigger
                // rescheduling.
                self->state = fiber_state::Dead;

#ifdef ABEL_INTERNAL_USE_ASAN
                self->asan_terminating = true;
#endif

                // We need to switch to master fiber and free the resources there, there's
                // no call-stack for us to return.
                get_master_fiber_entity()->resume_on([self, lk = std::move(ebl)]() mutable {
                    // The `exit_barrier` is move out so as to free `self` (the stack)
                    // earlier. Stack resource is precious.
                    auto eb = std::move(self->ref_exit_barrier);

                    // Because no one else if referring `self` (see comments above), we're
                    // safe to free it here.
                    free_fiber_entity(self);  // Good-bye.

                    // Were anyone waiting on us, wake them up now.
                    eb->unsafe_count_down(std::move(lk));
                });
            }
            DCHECK(0);  // Can't be here.
        }

        void fiber_entity::resume_on(abel::function<void()> &&cb) noexcept {
            auto caller = get_current_fiber_entity();
            DCHECK_MSG(!resume_proc,
                        "You may not call `ResumeOn` on a fiber twice (before the first "
                        "one has executed).");
            DCHECK_MSG(caller != this, "Calling `resume()` on self is undefined.");

            // This pending call will be performed and cleared immediately when we
            // switched to `*this` fiber (before calling user's continuation).
            resume_proc = std::move(cb);
            resume();
        }

        erased_ptr *fiber_entity::get_fls_slow(std::size_t index) noexcept {
           // ABEL_LOG_WARNING_ONCE(
            //        "Excessive FLS usage. Performance will likely degrade.");
            if (ABEL_UNLIKELY(!external_fls)) {
                external_fls =
                        std::make_unique<std::unordered_map<std::size_t, erased_ptr>>();
            }
            return &(*external_fls)[index];
        }

        fiber_entity::trivial_fls_t *fiber_entity::get_trivial_fls_slow(
                std::size_t index) noexcept {
            //ABEL_LOG_WARNING_ONCE(
             //       "Excessive FLS usage. Performance will likely degrade.");
            if (ABEL_UNLIKELY(!external_trivial_fls)) {
                external_trivial_fls =
                        std::make_unique<std::unordered_map<std::size_t, trivial_fls_t>>();
            }
            return &(*external_trivial_fls)[index];
        }

        void set_up_master_fiber_entity() noexcept {
            thread_local fiber_entity master_fiber_impl;

            master_fiber_impl.debugging_fiber_id = -1;
            //master_fiber_impl.state_save_area = nullptr;
            master_fiber_impl.state = fiber_state::Running;
            master_fiber_impl.stack_size = 0;

            master_fiber_impl.own_scheduling_group = scheduling_group::current();

#ifdef ABEL_INTERNAL_USE_ASAN
            std::tie(master_fiber_impl.asan_stack_bottom,
               master_fiber_impl.asan_stack_size) = get_master_fiber_stack();
#endif

#ifdef ABEL_INTERNAL_USE_TSAN
            master_fiber_impl.tsan_fiber = abel::tsan::get_current_fiber();
#endif

            master_fiber = &master_fiber_impl;
            set_current_fiber_entity(master_fiber);
        }

#if defined(ABEL_INTERNAL_USE_TSAN) || defined(__powerpc64__) || \
    defined(__aarch64__)

    fiber_entity* get_master_fiber_entity() noexcept { return master_fiber; }

    fiber_entity* get_current_fiber_entity() noexcept { return current_fiber; }

    void set_current_fiber_entity(fiber_entity* current) { current_fiber = current; }

#endif

        fiber_entity *create_fiber_entity(scheduling_group *sg, bool system_fiber,
                                       abel::function<void()> &&start_proc) noexcept {
            auto stack = system_fiber ? create_system_stack() : create_user_stack();
            auto stack_size =
                    system_fiber ? kSystemStackSize : FLAGS_fiber_stack_size;
            auto bottom = reinterpret_cast<char *>(stack) + stack_size;
            // `fiber_entity` (and magic) is stored at the stack bottom.
            auto ptr = bottom - kFiberStackReservedSize;
            DCHECK(reinterpret_cast<std::uintptr_t>(ptr) % alignof(fiber_entity) ==
                         0);
            // NOT value-initialized intentionally, to save precious CPU cycles.
            auto fiber = new(ptr) fiber_entity;  // A new life has born.

            fiber->debugging_fiber_id = abel::fiber_internal::next_id<fiber_id_traits>();
            // `fiber->ever_started_magic` is not filled here. @sa: `fiber_proc`.
            fiber->system_fiber = system_fiber;
            fiber->stack_size = stack_size - kFiberStackReservedSize;
            fiber->state_save_area =
                    fiber_make_context(fiber->get_stack_top(), fiber->get_stack_limit(), reinterpret_cast<void(*)(intptr_t)>(fiber_proc));

            //abel_fiber_set_target(&fiber->context, stack,  fiber->get_stack_limit() - kFiberStackReservedSize, fiber_proc, fiber);
            fiber->own_scheduling_group = sg;
            fiber->start_proc = std::move(start_proc);
            fiber->state = fiber_state::Ready;

#ifdef ABEL_INTERNAL_USE_ASAN
            // Using the lowest VA here is NOT a mistake.
      //
      // Despite the fact that we normally treat the bottom of the stack as the
      // highest VA possible (assuming the stack grows down), AFAICT from source
      // code of ASan, it does expect the lowest VA here.
      //
      // @sa: `gcc/libsanitizer/asan/asan_thread.cpp`:
      //
      // > 117 void AsanThread::StartSwitchFiber(...) {
      // ...
      // > 124   next_stack_bottom_ = bottom;
      // > 125   next_stack_top_ = bottom + size;
      // > 126   atomic_store(&stack_switching_, 1, memory_order_release);
      fiber->asan_stack_bottom = stack;

      // NOT `fiber->GetStackLimit()`.
      //
      // Reserved space is also made accessible to ASan. These bytes might be
      // accessed by the fiber later (e.g., `start_proc`.).
      fiber->asan_stack_size = stack_size;
#endif

#ifdef ABEL_INTERNAL_USE_TSAN
            fiber->tsan_fiber = abel::tsan::create_fiber();
#endif

            return fiber;
        }

        void free_fiber_entity(fiber_entity *fiber) noexcept {
            bool system_fiber = fiber->system_fiber;

#ifdef ABEL_INTERNAL_USE_TSAN
            abel::tsan::destroy_fiber(fiber->tsan_fiber);
#endif

            fiber->ever_started_magic = 0;  // Hopefully the compiler does not optimize
            // this away.
            fiber->~fiber_entity();

            auto p = reinterpret_cast<char *>(fiber) + kFiberStackReservedSize -
                     (system_fiber ? kSystemStackSize : FLAGS_fiber_stack_size);
            if (system_fiber) {
                free_system_stack(p);
            } else {
                free_user_stack(p);
            }
        }
    }  // namespace fiber_internal
}  // namespace abel
