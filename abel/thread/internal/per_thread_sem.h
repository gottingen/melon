

// PerThreadSem is a low-level synchronization primitive controlling the
// runnability of a single thread, used internally by mutex and cond_var.
//
// This is NOT a general-purpose synchronization mechanism, and should not be
// used directly by applications.  Applications should use mutex and cond_var.
//
// The semantics of PerThreadSem are the same as that of a counting semaphore.
// Each thread maintains an abstract "count" value associated with its identity.

#ifndef ABEL_SYNCHRONIZATION_INTERNAL_PER_THREAD_SEM_H_
#define ABEL_SYNCHRONIZATION_INTERNAL_PER_THREAD_SEM_H_

#include <atomic>

#include <abel/thread/internal/thread_identity.h>
#include <abel/thread/internal/create_thread_identity.h>
#include <abel/thread/internal/kernel_timeout.h>

namespace abel {


    class mutex;

    namespace thread_internal {

        class PerThreadSem {
        public:
            PerThreadSem() = delete;

            PerThreadSem(const PerThreadSem &) = delete;

            PerThreadSem &operator=(const PerThreadSem &) = delete;

            // Routine invoked periodically (once a second) by a background thread.
            // Has no effect on user-visible state.
            static void Tick(thread_internal::ThreadIdentity *identity);

            // ---------------------------------------------------------------------------
            // Routines used by autosizing threadpools to detect when threads are
            // blocked.  Each thread has a counter pointer, initially zero.  If non-zero,
            // the implementation atomically increments the counter when it blocks on a
            // semaphore, a decrements it again when it wakes.  This allows a threadpool
            // to keep track of how many of its threads are blocked.
            // SetThreadBlockedCounter() should be used only by threadpool
            // implementations.  GetThreadBlockedCounter() should be used by modules that
            // block threads; if the pointer returned is non-zero, the location should be
            // incremented before the thread blocks, and decremented after it wakes.
            static void SetThreadBlockedCounter(std::atomic<int> *counter);

            static std::atomic<int> *GetThreadBlockedCounter();

        private:
            // Create the PerThreadSem associated with "identity".  Initializes count=0.
            // REQUIRES: May only be called by ThreadIdentity.
            static void Init(thread_internal::ThreadIdentity *identity);

            // Destroy the PerThreadSem associated with "identity".
            // REQUIRES: May only be called by ThreadIdentity.
            static void Destroy(thread_internal::ThreadIdentity *identity);

            // Increments "identity"'s count.
            static ABEL_FORCE_INLINE void Post(thread_internal::ThreadIdentity *identity);

            // Waits until either our count > 0 or t has expired.
            // If count > 0, decrements count and returns true.  Otherwise returns false.
            // !t.has_timeout() => wait(t) will return true.
            static ABEL_FORCE_INLINE bool wait(KernelTimeout t);

            // White-listed callers.
            friend class PerThreadSemTest;

            friend class abel::mutex;

            friend abel::thread_internal::ThreadIdentity *CreateThreadIdentity();

            friend void ReclaimThreadIdentity(void *v);
        };

    }  // namespace thread_internal

}  // namespace abel

// In some build configurations we pass --detect-odr-violations to the
// gold linker.  This causes it to flag weak symbol overrides as ODR
// violations.  Because ODR only applies to C++ and not C,
// --detect-odr-violations ignores symbols not mangled with C++ names.
// By changing our extension points to be extern "C", we dodge this
// check.
extern "C" {
void AbelInternalPerThreadSemPost(
        abel::thread_internal::ThreadIdentity *identity);
bool AbelInternalPerThreadSemWait(
        abel::thread_internal::KernelTimeout t);
}  // extern "C"

void abel::thread_internal::PerThreadSem::Post(
        abel::thread_internal::ThreadIdentity *identity) {
    AbelInternalPerThreadSemPost(identity);
}

bool abel::thread_internal::PerThreadSem::wait(
        abel::thread_internal::KernelTimeout t) {
    return AbelInternalPerThreadSemWait(t);
}

#endif  // ABEL_SYNCHRONIZATION_INTERNAL_PER_THREAD_SEM_H_
