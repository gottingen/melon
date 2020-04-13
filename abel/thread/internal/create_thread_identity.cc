//

#include <stdint.h>
#include <new>

// This file is a no-op if the required low_level_alloc support is missing.
#include <abel/memory/internal/low_level_alloc.h>

#ifndef ABEL_LOW_LEVEL_ALLOC_MISSING

#include <string.h>

#include <abel/base/profile.h>
#include <abel/thread/internal/spinlock.h>
#include <abel/thread/internal/thread_identity.h>
#include <abel/thread/internal/per_thread_sem.h>

namespace abel {

    namespace thread_internal {

// ThreadIdentity storage is persistent, we maintain a free-list of previously
// released ThreadIdentity objects.
        static thread_internal::SpinLock freelist_lock(
                base_internal::kLinkerInitialized);
        static thread_internal::ThreadIdentity *thread_identity_freelist;

// A per-thread destructor for reclaiming associated ThreadIdentity objects.
// Since we must preserve their storage we cache them for re-use.
        void ReclaimThreadIdentity(void *v) {
            thread_internal::ThreadIdentity *identity =
                    static_cast<thread_internal::ThreadIdentity *>(v);

            // all_locks might have been allocated by the mutex implementation.
            // We free it here when we are notified that our thread is dying.
            if (identity->per_thread_synch.all_locks != nullptr) {
                memory_internal::low_level_alloc::free(identity->per_thread_synch.all_locks);
            }

            PerThreadSem::Destroy(identity);

            // We must explicitly clear the current thread's identity:
            // (a) Subsequent (unrelated) per-thread destructors may require an identity.
            //     We must guarantee a new identity is used in this case (this instructor
            //     will be reinvoked up to PTHREAD_DESTRUCTOR_ITERATIONS in this case).
            // (b) ThreadIdentity implementations may depend on memory that is not
            //     reinitialized before reuse.  We must allow explicit clearing of the
            //     association state in this case.
            thread_internal::ClearCurrentThreadIdentity();
            {
                thread_internal::SpinLockHolder l(&freelist_lock);
                identity->next = thread_identity_freelist;
                thread_identity_freelist = identity;
            }
        }

// Return value rounded up to next multiple of align.
// Align must be a power of two.
        static intptr_t RoundUp(intptr_t addr, intptr_t align) {
            return (addr + align - 1) & ~(align - 1);
        }

        static void ResetThreadIdentity(thread_internal::ThreadIdentity *identity) {
            thread_internal::PerThreadSynch *pts = &identity->per_thread_synch;
            pts->next = nullptr;
            pts->skip = nullptr;
            pts->may_skip = false;
            pts->waitp = nullptr;
            pts->suppress_fatal_errors = false;
            pts->readers = 0;
            pts->priority = 0;
            pts->next_priority_read_cycles = 0;
            pts->state.store(thread_internal::PerThreadSynch::State::kAvailable,
                             std::memory_order_relaxed);
            pts->maybe_unlocking = false;
            pts->wake = false;
            pts->cond_waiter = false;
            pts->all_locks = nullptr;
            identity->blocked_count_ptr = nullptr;
            identity->ticker.store(0, std::memory_order_relaxed);
            identity->wait_start.store(0, std::memory_order_relaxed);
            identity->is_idle.store(false, std::memory_order_relaxed);
            identity->next = nullptr;
        }

        static thread_internal::ThreadIdentity *NewThreadIdentity() {
            thread_internal::ThreadIdentity *identity = nullptr;

            {
                // Re-use a previously released object if possible.
                thread_internal::SpinLockHolder l(&freelist_lock);
                if (thread_identity_freelist) {
                    identity = thread_identity_freelist;  // Take list-head.
                    thread_identity_freelist = thread_identity_freelist->next;
                }
            }

            if (identity == nullptr) {
                // Allocate enough space to align ThreadIdentity to a multiple of
                // PerThreadSynch::kAlignment. This space is never released (it is
                // added to a freelist by ReclaimThreadIdentity instead).
                void *allocation = memory_internal::low_level_alloc::alloc(
                        sizeof(*identity) + thread_internal::PerThreadSynch::kAlignment - 1);
                // Round up the address to the required alignment.
                identity = reinterpret_cast<thread_internal::ThreadIdentity *>(
                        RoundUp(reinterpret_cast<intptr_t>(allocation),
                                thread_internal::PerThreadSynch::kAlignment));
            }
            ResetThreadIdentity(identity);

            return identity;
        }

// Allocates and attaches ThreadIdentity object for the calling thread.  Returns
// the new identity.
// REQUIRES: CurrentThreadIdentity(false) == nullptr
        thread_internal::ThreadIdentity *CreateThreadIdentity() {
            thread_internal::ThreadIdentity *identity = NewThreadIdentity();
            PerThreadSem::Init(identity);
            // Associate the value with the current thread, and attach our destructor.
            thread_internal::SetCurrentThreadIdentity(identity, ReclaimThreadIdentity);
            return identity;
        }

    }  // namespace thread_internal

}  // namespace abel

#endif  // ABEL_LOW_LEVEL_ALLOC_MISSING
