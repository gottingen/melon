//

#include <abel/thread/internal/thread_identity.h>

#ifndef _WIN32

#include <pthread.h>
#include <signal.h>

#endif

#include <atomic>
#include <cassert>
#include <memory>

#include <abel/asl/functional/call_once.h>
#include <abel/log/abel_logging.h>
#include <abel/thread/internal/spinlock.h>

namespace abel {

    namespace thread_internal {

        namespace {
// Used to co-ordinate one-time creation of our pthread_key
            abel::once_flag init_thread_identity_key_once;
            pthread_key_t thread_identity_pthread_key;
            std::atomic<bool> pthread_key_initialized(false);

            void AllocateThreadIdentityKey(ThreadIdentityReclaimerFunction reclaimer) {
                pthread_key_create(&thread_identity_pthread_key, reclaimer);
                pthread_key_initialized.store(true, std::memory_order_release);
            }
        }  // namespace

        // The actual TLS storage for a thread's currently associated thread_identity.
        // This is referenced by inline accessors in the header.
        // "protected" visibility ensures that if multiple instances of abel code
        // exist within a process (via dlopen() or similar), references to
        // thread_identity_ptr from each instance of the code will refer to
        // *different* instances of this ptr.
#ifdef __GNUC__
        __attribute__((visibility("protected")))
#endif  // __GNUC__

        // Prefer __thread to thread_local as benchmarks indicate it is a bit faster.
        ABEL_THREAD_LOCAL thread_identity* thread_identity_ptr = nullptr;

        void SetCurrentThreadIdentity(
                thread_identity *identity, ThreadIdentityReclaimerFunction reclaimer) {
            assert(CurrentThreadIdentityIfPresent() == nullptr);
            // Associate our destructor.
            // NOTE: This call to pthread_setspecific is currently the only immovable
            // barrier to CurrentThreadIdentity() always being async signal safe.
            // NOTE: Not async-safe.  But can be open-coded.
            abel::call_once(init_thread_identity_key_once, AllocateThreadIdentityKey,
                            reclaimer);
            pthread_setspecific(thread_identity_pthread_key,
                                reinterpret_cast<void*>(identity));
            thread_identity_ptr = identity;
        }

        void ClearCurrentThreadIdentity() {
            thread_identity_ptr = nullptr;
        }


    }  // namespace thread_internal

}  // namespace abel
