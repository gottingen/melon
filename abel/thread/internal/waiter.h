//

#ifndef ABEL_SYNCHRONIZATION_INTERNAL_WAITER_H_
#define ABEL_SYNCHRONIZATION_INTERNAL_WAITER_H_

#include <abel/base/profile.h>

#ifdef _WIN32
#include <sdkddkver.h>
#else

#include <pthread.h>

#endif

#ifdef __linux__
#include <linux/futex.h>
#endif

#ifdef ABEL_HAVE_SEMAPHORE_H

#include <semaphore.h>

#endif

#include <atomic>
#include <cstdint>

#include <abel/thread/internal/thread_identity.h>
#include <abel/thread/internal/kernel_timeout.h>

// May be chosen at compile time via -DABEL_FORCE_WAITER_MODE=<index>
#define ABEL_WAITER_MODE_FUTEX 0
#define ABEL_WAITER_MODE_SEM 1
#define ABEL_WAITER_MODE_CONDVAR 2
#define ABEL_WAITER_MODE_WIN32 3

#if defined(ABEL_FORCE_WAITER_MODE)
#define ABEL_WAITER_MODE ABEL_FORCE_WAITER_MODE
#elif defined(_WIN32) && _WIN32_WINNT >= _WIN32_WINNT_VISTA
#define ABEL_WAITER_MODE ABEL_WAITER_MODE_WIN32
#elif defined(__BIONIC__)
// Bionic supports all the futex operations we need even when some of the futex
// definitions are missing.
#define ABEL_WAITER_MODE ABEL_WAITER_MODE_FUTEX
#elif defined(__linux__) && defined(FUTEX_CLOCK_REALTIME)
// FUTEX_CLOCK_REALTIME requires Linux >= 2.6.28.
#define ABEL_WAITER_MODE ABEL_WAITER_MODE_FUTEX
#elif defined(ABEL_HAVE_SEMAPHORE_H) && !defined(ABEL_PLATFORM_APPLE)
#define ABEL_WAITER_MODE ABEL_WAITER_MODE_SEM
#else
#define ABEL_WAITER_MODE ABEL_WAITER_MODE_CONDVAR
#endif

namespace abel {

    namespace thread_internal {

// Waiter is an OS-specific semaphore.
        class Waiter {
        public:
            // Prepare any data to track waits.
            Waiter();

            // Not copyable or movable
            Waiter(const Waiter &) = delete;

            Waiter &operator=(const Waiter &) = delete;

            // Destroy any data to track waits.
            ~Waiter();

            // Blocks the calling thread until a matching call to `Post()` or
            // `t` has passed. Returns `true` if woken (`Post()` called),
            // `false` on timeout.
            bool wait(KernelTimeout t);

            // Restart the caller of `wait()` as with a normal semaphore.
            void Post();

            // If anyone is waiting, wake them up temporarily and cause them to
            // call `MaybeBecomeIdle()`. They will then return to waiting for a
            // `Post()` or timeout.
            void Poke();

            // Returns the Waiter associated with the identity.
            static Waiter *GetWaiter(thread_internal::ThreadIdentity *identity) {
                static_assert(
                        sizeof(Waiter) <= sizeof(thread_internal::ThreadIdentity::WaiterState),
                        "Insufficient space for Waiter");
                return reinterpret_cast<Waiter *>(identity->waiter_state.data);
            }

            // How many periods to remain idle before releasing resources
#ifndef THREAD_SANITIZER
            static const int kIdlePeriods = 60;
#else
            // Memory consumption under ThreadSanitizer is a serious concern,
            // so we release resources sooner. The value of 1 leads to 1 to 2 second
            // delay before marking a thread as idle.
            static const int kIdlePeriods = 1;
#endif

        private:
#if ABEL_WAITER_MODE == ABEL_WAITER_MODE_FUTEX
            // Futexes are defined by specification to be 32-bits.
            // Thus std::atomic<int32_t> must be just an int32_t with lockfree methods.
            std::atomic<int32_t> futex_;
            static_assert(sizeof(int32_t) == sizeof(futex_), "Wrong size for futex");

#elif ABEL_WAITER_MODE == ABEL_WAITER_MODE_CONDVAR

            // REQUIRES: mu_ must be held.
            void InternalCondVarPoke();

            pthread_mutex_t mu_;
            pthread_cond_t cv_;
            int waiter_count_;
            int wakeup_count_;  // Unclaimed wakeups.

#elif ABEL_WAITER_MODE == ABEL_WAITER_MODE_SEM
            sem_t sem_;
            // This seems superfluous, but for Poke() we need to cause spurious
            // wakeups on the semaphore. Hence we can't actually use the
            // semaphore's count.
            std::atomic<int> wakeups_;

#elif ABEL_WAITER_MODE == ABEL_WAITER_MODE_WIN32
            // We can't include Windows.h in our headers, so we use aligned storage
            // buffers to define the storage of SRWLOCK and CONDITION_VARIABLE.
            using SRWLockStorage =
                typename std::aligned_storage<sizeof(void*), alignof(void*)>::type;
            using ConditionVariableStorage =
                typename std::aligned_storage<sizeof(void*), alignof(void*)>::type;

            // WinHelper - Used to define utilities for accessing the lock and
            // condition variable storage once the types are complete.
            class WinHelper;

            // REQUIRES: WinHelper::GetLock(this) must be held.
            void InternalCondVarPoke();

            SRWLockStorage mu_storage_;
            ConditionVariableStorage cv_storage_;
            int waiter_count_;
            int wakeup_count_;

#else
#error Unknown ABEL_WAITER_MODE
#endif
        };

    }  // namespace thread_internal

}  // namespace abel

#endif  // ABEL_SYNCHRONIZATION_INTERNAL_WAITER_H_
