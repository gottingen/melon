//
//  Copyright 2019 The Abseil Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

//  Most users requiring mutual exclusion should use mutex.
//  spin_lock is provided for use in three situations:
//   - for use in code that mutex itself depends on
//   - to get a faster fast-path release under low contention (without an
//     atomic read-modify-write) In return, spin_lock has worse behaviour under
//     contention, which is why mutex is preferred in most situations.
//   - for async signal safety (see below)

// spin_lock is async signal safe.  If a spinlock is used within a signal
// handler, all code that acquires the lock must ensure that the signal cannot
// arrive while they are holding the lock.  Typically, this is done by blocking
// the signal.

#ifndef ABEL_THREADING_INTERNAL_SPINLOCK_H_
#define ABEL_THREADING_INTERNAL_SPINLOCK_H_

#include <cstdint>
#include <sys/types.h>

#include <atomic>

#include <abel/base/profile.h>
#include <abel/thread/dynamic_annotations.h>
#include <abel/thread/internal/low_level_scheduling.h>
#include <abel/log/abel_logging.h>
#include <abel/thread/internal/scheduling_mode.h>
#include <abel/thread/internal/tsan_mutex_interface.h>
#include <abel/base/profile.h>
#include <abel/thread/thread_annotations.h>

namespace abel {

    namespace thread_internal {

        class ABEL_LOCKABLE spin_lock {
        public:
            spin_lock() : lockword_(kSpinLockCooperative) {
                ABEL_TSAN_MUTEX_CREATE(this, __tsan_mutex_not_static);
            }

            // Special constructor for use with static spin_lock objects.  E.g.,
            //
            //    static spin_lock lock(base_internal::kLinkerInitialized);
            //
            // When initialized using this constructor, we depend on the fact
            // that the linker has already initialized the memory appropriately. The lock
            // is initialized in non-cooperative mode.
            //
            // A spin_lock constructed like this can be freely used from global
            // initializers without worrying about the order in which global
            // initializers run.
            explicit spin_lock(base_internal::LinkerInitialized) {
                // Does nothing; lockword_ is already initialized
                ABEL_TSAN_MUTEX_CREATE(this, 0);
            }

            // Constructors that allow non-cooperative spinlocks to be created for use
            // inside thread schedulers.  Normal clients should not use these.
            explicit spin_lock(thread_internal::SchedulingMode mode);

            spin_lock(base_internal::LinkerInitialized,
                     thread_internal::SchedulingMode mode);

            ~spin_lock() { ABEL_TSAN_MUTEX_DESTROY(this, __tsan_mutex_not_static); }

            // Acquire this spin_lock.
            ABEL_FORCE_INLINE void lock() ABEL_EXCLUSIVE_LOCK_FUNCTION() {
                ABEL_TSAN_MUTEX_PRE_LOCK(this, 0);
                if (!TryLockImpl()) {
                    SlowLock();
                }
                ABEL_TSAN_MUTEX_POST_LOCK(this, 0, 0);
            }

            // Try to acquire this spin_lock without blocking and return true if the
            // acquisition was successful.  If the lock was not acquired, false is
            // returned.  If this spin_lock is free at the time of the call, try_lock
            // will return true with high probability.
            ABEL_FORCE_INLINE bool try_lock() ABEL_EXCLUSIVE_TRYLOCK_FUNCTION(true) {
                ABEL_TSAN_MUTEX_PRE_LOCK(this, __tsan_mutex_try_lock);
                bool res = TryLockImpl();
                ABEL_TSAN_MUTEX_POST_LOCK(
                        this, __tsan_mutex_try_lock | (res ? 0 : __tsan_mutex_try_lock_failed),
                        0);
                return res;
            }

            // Release this spin_lock, which must be held by the calling thread.
            ABEL_FORCE_INLINE void unlock() ABEL_UNLOCK_FUNCTION() {
                ABEL_TSAN_MUTEX_PRE_UNLOCK(this, 0);
                uint32_t lock_value = lockword_.load(std::memory_order_relaxed);
                lock_value = lockword_.exchange(lock_value & kSpinLockCooperative,
                                                std::memory_order_release);

                if ((lock_value & kSpinLockDisabledScheduling) != 0) {
                    thread_internal::scheduling_guard::enable_rescheduling(true);
                }
                if ((lock_value & kWaitTimeMask) != 0) {
                    // Collect contentionz profile info, and speed the wakeup of any waiter.
                    // The wait_cycles value indicates how long this thread spent waiting
                    // for the lock.
                    SlowUnlock(lock_value);
                }
                ABEL_TSAN_MUTEX_POST_UNLOCK(this, 0);
            }

            // Determine if the lock is held.  When the lock is held by the invoking
            // thread, true will always be returned. Intended to be used as
            // CHECK(lock.IsHeld()).
            ABEL_FORCE_INLINE bool IsHeld() const {
                return (lockword_.load(std::memory_order_relaxed) & kSpinLockHeld) != 0;
            }

        protected:
            // These should not be exported except for testing.

            // Store number of cycles between wait_start_time and wait_end_time in a
            // lock value.
            static uint32_t EncodeWaitCycles(int64_t wait_start_time,
                                             int64_t wait_end_time);

            // Extract number of wait cycles in a lock value.
            static uint64_t DecodeWaitCycles(uint32_t lock_value);

            // Provide access to protected method above.  Use for testing only.
            friend struct SpinLockTest;

        private:
            // lockword_ is used to store the following:
            //
            // bit[0] encodes whether a lock is being held.
            // bit[1] encodes whether a lock uses cooperative scheduling.
            // bit[2] encodes whether a lock disables scheduling.
            // bit[3:31] encodes time a lock spent on waiting as a 29-bit unsigned int.
            enum {
                kSpinLockHeld = 1
            };
            enum {
                kSpinLockCooperative = 2
            };
            enum {
                kSpinLockDisabledScheduling = 4
            };
            enum {
                kSpinLockSleeper = 8
            };
            enum {
                kWaitTimeMask =                      // Includes kSpinLockSleeper.
                ~(kSpinLockHeld | kSpinLockCooperative | kSpinLockDisabledScheduling)
            };

            // Returns true if the provided scheduling mode is cooperative.
            static constexpr bool IsCooperative(
                    thread_internal::SchedulingMode scheduling_mode) {
                return scheduling_mode == thread_internal::SCHEDULE_COOPERATIVE_AND_KERNEL;
            }

            uint32_t TryLockInternal(uint32_t lock_value, uint32_t wait_cycles);

            void InitLinkerInitializedAndCooperative();

            void SlowLock() ABEL_COLD;

            void SlowUnlock(uint32_t lock_value) ABEL_COLD;

            uint32_t SpinLoop();

            ABEL_FORCE_INLINE bool TryLockImpl() {
                uint32_t lock_value = lockword_.load(std::memory_order_relaxed);
                return (TryLockInternal(lock_value, 0) & kSpinLockHeld) == 0;
            }

            std::atomic<uint32_t> lockword_;

            spin_lock(const spin_lock &) = delete;

            spin_lock &operator=(const spin_lock &) = delete;
        };

// Corresponding locker object that arranges to acquire a spinlock for
// the duration of a C++ scope.
        class ABEL_SCOPED_LOCKABLE SpinLockHolder {
        public:
            ABEL_FORCE_INLINE explicit SpinLockHolder(spin_lock *l) ABEL_EXCLUSIVE_LOCK_FUNCTION(l)
                    : lock_(l) {
                l->lock();
            }

            ABEL_FORCE_INLINE ~SpinLockHolder() ABEL_UNLOCK_FUNCTION() { lock_->unlock(); }

            SpinLockHolder(const SpinLockHolder &) = delete;

            SpinLockHolder &operator=(const SpinLockHolder &) = delete;

        private:
            spin_lock *lock_;
        };

// Register a hook for profiling support.
//
// The function pointer registered here will be called whenever a spinlock is
// contended.  The callback is given an opaque handle to the contended spinlock
// and the number of wait cycles.  This is thread-safe, but only a single
// profiler can be registered.  It is an error to call this function multiple
// times with different arguments.
        void RegisterSpinLockProfiler(void (*fn)(const void *lock,
                                                 int64_t wait_cycles));

//------------------------------------------------------------------------------
// Public interface ends here.
//------------------------------------------------------------------------------

// If (result & kSpinLockHeld) == 0, then *this was successfully locked.
// Otherwise, returns last observed value for lockword_.
        ABEL_FORCE_INLINE uint32_t spin_lock::TryLockInternal(uint32_t lock_value,
                                                             uint32_t wait_cycles) {
            if ((lock_value & kSpinLockHeld) != 0) {
                return lock_value;
            }

            uint32_t sched_disabled_bit = 0;
            if ((lock_value & kSpinLockCooperative) == 0) {
                // For non-cooperative locks we must make sure we mark ourselves as
                // non-reschedulable before we attempt to CompareAndSwap.
                if (thread_internal::scheduling_guard::disable_rescheduling()) {
                    sched_disabled_bit = kSpinLockDisabledScheduling;
                }
            }

            if (!lockword_.compare_exchange_strong(
                    lock_value,
                    kSpinLockHeld | lock_value | wait_cycles | sched_disabled_bit,
                    std::memory_order_acquire, std::memory_order_relaxed)) {
                thread_internal::scheduling_guard::enable_rescheduling(sched_disabled_bit != 0);
            }

            return lock_value;
        }

    }  // namespace thread_internal

}  // namespace abel

#endif  // ABEL_THREADING_INTERNAL_SPINLOCK_H_
