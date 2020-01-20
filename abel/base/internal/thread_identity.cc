//

#include <abel/base/internal/thread_identity.h>

#ifndef _WIN32
#include <pthread.h>
#include <signal.h>
#endif

#include <atomic>
#include <cassert>
#include <memory>

#include <abel/functional/call_once.h>
#include <abel/base/internal/raw_logging.h>
#include <abel/base/internal/spinlock.h>

namespace abel {

namespace base_internal {

#if ABEL_THREAD_IDENTITY_MODE != ABEL_THREAD_IDENTITY_MODE_USE_CPP11
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
#endif

#if ABEL_THREAD_IDENTITY_MODE == ABEL_THREAD_IDENTITY_MODE_USE_TLS || \
    ABEL_THREAD_IDENTITY_MODE == ABEL_THREAD_IDENTITY_MODE_USE_CPP11
// The actual TLS storage for a thread's currently associated ThreadIdentity.
// This is referenced by inline accessors in the header.
// "protected" visibility ensures that if multiple instances of abel code
// exist within a process (via dlopen() or similar), references to
// thread_identity_ptr from each instance of the code will refer to
// *different* instances of this ptr.
#ifdef __GNUC__
__attribute__((visibility("protected")))
#endif  // __GNUC__
#if ABEL_PER_THREAD_TLS
// Prefer __thread to thread_local as benchmarks indicate it is a bit faster.
ABEL_PER_THREAD_TLS_KEYWORD ThreadIdentity* thread_identity_ptr = nullptr;
#elif defined(ABEL_HAVE_THREAD_LOCAL)
thread_local ThreadIdentity* thread_identity_ptr = nullptr;
#endif  // ABEL_PER_THREAD_TLS
#endif  // TLS or CPP11

void SetCurrentThreadIdentity(
    ThreadIdentity* identity, ThreadIdentityReclaimerFunction reclaimer) {
  assert(CurrentThreadIdentityIfPresent() == nullptr);
  // Associate our destructor.
  // NOTE: This call to pthread_setspecific is currently the only immovable
  // barrier to CurrentThreadIdentity() always being async signal safe.
#if ABEL_THREAD_IDENTITY_MODE == ABEL_THREAD_IDENTITY_MODE_USE_POSIX_SETSPECIFIC
  // NOTE: Not async-safe.  But can be open-coded.
  abel::call_once(init_thread_identity_key_once, AllocateThreadIdentityKey,
                  reclaimer);

#if defined(__EMSCRIPTEN__) || defined(__MINGW32__)
  // Emscripten and MinGW pthread implementations does not support signals.
  // See https://kripken.github.io/emscripten-site/docs/porting/pthreads.html
  // for more information.
  pthread_setspecific(thread_identity_pthread_key,
                      reinterpret_cast<void*>(identity));
#else
  // We must mask signals around the call to setspecific as with current glibc,
  // a concurrent getspecific (needed for GetCurrentThreadIdentityIfPresent())
  // may zero our value.
  //
  // While not officially async-signal safe, getspecific within a signal handler
  // is otherwise OK.
  sigset_t all_signals;
  sigset_t curr_signals;
  sigfillset(&all_signals);
  pthread_sigmask(SIG_SETMASK, &all_signals, &curr_signals);
  pthread_setspecific(thread_identity_pthread_key,
                      reinterpret_cast<void*>(identity));
  pthread_sigmask(SIG_SETMASK, &curr_signals, nullptr);
#endif  // !__EMSCRIPTEN__ && !__MINGW32__

#elif ABEL_THREAD_IDENTITY_MODE == ABEL_THREAD_IDENTITY_MODE_USE_TLS
  // NOTE: Not async-safe.  But can be open-coded.
  abel::call_once(init_thread_identity_key_once, AllocateThreadIdentityKey,
                  reclaimer);
  pthread_setspecific(thread_identity_pthread_key,
                      reinterpret_cast<void*>(identity));
  thread_identity_ptr = identity;
#elif ABEL_THREAD_IDENTITY_MODE == ABEL_THREAD_IDENTITY_MODE_USE_CPP11
  thread_local std::unique_ptr<ThreadIdentity, ThreadIdentityReclaimerFunction>
      holder(identity, reclaimer);
  thread_identity_ptr = identity;
#else
#error Unimplemented ABEL_THREAD_IDENTITY_MODE
#endif
}

void ClearCurrentThreadIdentity() {
#if ABEL_THREAD_IDENTITY_MODE == ABEL_THREAD_IDENTITY_MODE_USE_TLS || \
    ABEL_THREAD_IDENTITY_MODE == ABEL_THREAD_IDENTITY_MODE_USE_CPP11
  thread_identity_ptr = nullptr;
#elif ABEL_THREAD_IDENTITY_MODE == \
      ABEL_THREAD_IDENTITY_MODE_USE_POSIX_SETSPECIFIC
  // pthread_setspecific expected to clear value on destruction
  assert(CurrentThreadIdentityIfPresent() == nullptr);
#endif
}

#if ABEL_THREAD_IDENTITY_MODE == ABEL_THREAD_IDENTITY_MODE_USE_POSIX_SETSPECIFIC
ThreadIdentity* CurrentThreadIdentityIfPresent() {
  bool initialized = pthread_key_initialized.load(std::memory_order_acquire);
  if (!initialized) {
    return nullptr;
  }
  return reinterpret_cast<ThreadIdentity*>(
      pthread_getspecific(thread_identity_pthread_key));
}
#endif

}  // namespace base_internal

}  // namespace abel
