//
// Created by liyinbin on 2021/4/4.
//

#ifndef ABEL_FIBER_INTERNAL_WAITABLE_H_
#define ABEL_FIBER_INTERNAL_WAITABLE_H_


#include <chrono>
#include <limits>
#include <mutex>
#include <utility>
#include <vector>

#include "abel/fiber/internal/fiber_entity.h"
#include "abel/log/logging.h"
#include "abel/base/profile.h"
#include "abel/memory/ref_ptr.h"
#include "abel/fiber/internal/spin_lock.h"
#include "abel/chrono/clock.h"
#include "abel/container/doubly_linked_list.h"

namespace abel {
    namespace fiber_internal {
        struct fiber_entity;

        class scheduling_group;

        // For waiting on an object, one or more `Waiter` structure should be allocated
        // on waiter's stack for chaining into `waitable`'s internal list.
        //
        // If the waiter is waiting on multiple `waitable`s and has waken up by one of
        // them, it's the waiter's responsibility to remove itself from the remaining
        // `waitable`s before continue running (so as not to induce use-after-free.).
        struct wait_block {
            fiber_entity *waiter = nullptr;  // This initialization will be optimized away.
            abel::doubly_linked_list_entry chain;
            std::atomic<bool> satisfied = false;
        };

        // Basic class for implementing waitable classes.
        //
        // Do NOT use this class directly, it's meant to be used as a building block.
        //
        // Thread-safe.
        class waitable {
        public:
            waitable() = default;

            ~waitable() { DCHECK(waiters_.empty(), "{}", waiters_.size()); }

            // Add a waiter to the tail.
            //
            // Returns `true` if the waiter is added to the wait chain, returns
            // `false` if the wait is immediately satisfied.
            //
            // To prevent wake-up loss, `fiber_entity::scheduler_lock` must be held by the
            // caller. (Otherwise before you take the lock, the fiber could have been
            // concurrently waken up, which is lost, by someone else.)
            bool add_waiter(wait_block *waiter);

            // Remove a waiter.
            //
            // Returns false if the waiter is not linked.
            bool try_remove_waiter(wait_block *waiter);

            // Popup one waiter and schedule it.
            //
            // Returns `nullptr` if there's no waiter.
            fiber_entity *wake_one();

            // Set this `waitable` as "persistently" awakened. After this call, all
            // further calls to `add_waiter` will fail.
            //
            // Pending waiters, if any, are returned.
            //
            // Be careful if you want to touch `waitable` after calling this method. If
            // someone else failed `add_waiter`, it would believe the wait was satisfied
            // immediately and could have freed this `waitable` before you touch it again.
            //
            // Normally you should call `WakeAll` after calling this method.
            std::vector<fiber_entity *> SetPersistentAwakened();

            // Undo `SetPersistentAwakened()`.
            void ResetAwakened();

            // Noncopyable, nonmovable.
            waitable(const waitable &) = delete;

            waitable &operator=(const waitable &) = delete;

        private:
            abel::fiber_internal::spinlock lock_;
            bool persistent_awakened_ = false;
            abel::doubly_linked_list<wait_block, &wait_block::chain> waiters_;
        };

        // "waitable" timer. This `waitable` signals all its waiters once the given time
        // point is reached.
        class waitable_timer {
        public:
            explicit waitable_timer(abel::time_point expires_at);

            ~waitable_timer();

            // Wait until the given time point is reached.
            void wait();

        private:
            // Reference counted `waitable.
            struct waitable_ref_counted : waitable, ref_counted<waitable_ref_counted> {
            };

            // This callback is implemented as a static method since the `waitable_timer`
            // object can be destroyed (especially if the timer is allocated on waiter's
            // stack) before this method returns.
            static void on_timer_expired(ref_ptr<waitable_ref_counted> ref);

        private:
            scheduling_group *sg_;
            std::uint64_t timer_id_;

            // Protects `impl_`.
            abel::fiber_internal::spinlock lock_;

            // We need to make this waitable ref-counted, otherwise if:
            //
            // - The user exits its fiber after awakened but before we finished with
            //   `waitable`, and
            //
            // - The `waitable` is allocated from user's stack.
            //
            // We'll be in trouble.
            ref_ptr<waitable_ref_counted> impl_;
        };

        // fiber_mutex for fiber.
        class fiber_mutex {
        public:
            bool try_lock() {
                DCHECK(is_fiber_context_present());

                std::uint32_t expected = 0;
                return count_.compare_exchange_strong(expected, 1,
                                                      std::memory_order_acquire);
            }

            void lock() {
                DCHECK(is_fiber_context_present());

                if (ABEL_LIKELY(try_lock())) {
                    return;
                }
                lock_slow();
            }

            void unlock();

        private:
            void lock_slow();

        private:
            waitable impl_;

            // Synchronizes between slow path of `lock()` and `unlock()`.
            abel::fiber_internal::spinlock slow_path_lock_;

            // Number of waiters (plus the owner). Hopefully `std::uint32_t` is large
            // enough.
            std::atomic<std::uint32_t> count_{0};
        };

        // Condition variable for fiber.
        class fiber_cond {
        public:
            void wait(std::unique_lock<fiber_mutex> &lock);

            template<class F>
            void wait(std::unique_lock<fiber_mutex> &lock, F &&pred) {
                DCHECK(is_fiber_context_present());

                while (!std::forward<F>(pred)()) {
                    wait(lock);
                }
                DCHECK(lock.owns_lock());
            }

            // You can always assume this method returns as a result of `notify_xxx` even
            // if it can actually results from timing out. This is conformant behavior --
            // it's just a spurious wake up in latter case.
            //
            // Returns `false` on timeout.
            bool wait_until(std::unique_lock<fiber_mutex> &lock,
                            abel::time_point expires_at);

            template<class F>
            bool wait_until(std::unique_lock<fiber_mutex> &lk,
                            abel::time_point timeout, F &&pred) {
                DCHECK(is_fiber_context_present());

                while (!std::forward<F>(pred)()) {
                    wait_until(lk, timeout);
                    if (abel::time_now() >= timeout) {
                        return std::forward<F>(pred)();
                    }
                }
                DCHECK(lk.owns_lock());
                return true;
            }

            void notify_one() noexcept;

            void notify_all() noexcept;

        private:
            waitable impl_;
        };

        // exit_barrier.
        //
        // This is effectively a `Latch` to implement `Fiber::join()`. However, unlike
        // `Latch`, we cannot afford to block (which can, in case of `Latch`, since
        // `count_down()` internally grabs lock) in waking up fibers (in master fiber.).
        //
        // Therefore we implement this class by separating grabbing the lock and wake up
        // waiters, so that the user can grab the lock in advance, avoiding block in
        // master fiber.
        class exit_barrier : public pool_ref_counted<exit_barrier> {
        public:
            exit_barrier();

            // Grab the lock required by `UnsafeCountDown()` in advance.
            std::unique_lock<fiber_mutex> GrabLock();

            // Count down the barrier's internal counter and wake up waiters.
            void unsafe_count_down(std::unique_lock<fiber_mutex> lk);

            // Won't block.
            void wait();

            void reset() { count_ = 1; }

        private:
            fiber_mutex lock_;
            std::size_t count_;
            fiber_cond cv_;
        };

        // Emulates Event in Win32 API.
        //
        // For internal use only. Users should stick with `fiber_mutex` + `fiber_cond`
        // instead.
        class wait_event {
        public:
            // Wait until `Set()` is called. If `Set()` is called before `Wait()`, this
            // method returns immediately.
            void wait();

            // Wake up fibers blocking on `Wait()`. All subsequent calls to `Wait()` will
            // return immediately.
            //
            // It's explicitly allowed to call this method outside of fiber context.
            void set();

        private:
            waitable impl_;
        };

        // This event type supports timeout, to some extent.
        //
        // For internal use only. Users should stick with `fiber_mutex` + `fiber_cond`
        // instead.
        class oneshot_timed_event {
        public:
            // The event is automatically set when `expires_at` is reached (and `Set` has
            // not been called).
            //
            // This type may only be instantiated in fiber context.
            explicit oneshot_timed_event(abel::time_point expires_at);

            ~oneshot_timed_event();

            // Wait until `Set()` has been called or timeout has expired.
            void wait();

            // Wake up any fibers blocking on `Wait()` (if the timeout has not expired
            // yet).
            //
            // It's explicitly allowed to call this method outside of fiber context.
            void set();

        private:
            struct Impl : ref_counted<Impl> {
                std::atomic<bool> event_set_guard{false};
                wait_event event;

                // Sets `event`. Calling this method multiple times is explicitly allowed.
                void IdempotentSet();
            };

            // Implemented as static method for the same reason as `waitable_timer`.
            static void on_timer_expired(ref_ptr<Impl> ref);

        private:
            scheduling_group *sg_;
            std::uint64_t timer_id_;

            ref_ptr<Impl> impl_;
        };
    }  // namespace fiber_internal

}  // namespace abel

namespace abel {

    template<>
    struct pool_traits<abel::fiber_internal::exit_barrier> {
        static constexpr auto kType = pool_type::ThreadLocal;
        static constexpr auto kLowWaterMark = 32768;
        static constexpr auto kHighWaterMark =
                std::numeric_limits<std::size_t>::max();
        static constexpr auto kMaxIdle = abel::duration::seconds(10);
        static constexpr auto kMinimumThreadCacheSize = 8192;
        static constexpr auto kTransferBatchSize = 1024;

        static void OnGet(abel::fiber_internal::exit_barrier *e) { e->reset(); }
    };

}  // namespace abel

#endif  // ABEL_FIBER_INTERNAL_WAITABLE_H_
