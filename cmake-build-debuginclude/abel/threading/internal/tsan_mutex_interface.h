//
// This file is intended solely for spinlock.h.
// It provides ThreadSanitizer annotations for custom mutexes.
// See <sanitizer/tsan_interface.h> for meaning of these annotations.

#ifndef ABEL_THREADING_INTERNAL_TSAN_MUTEX_INTERFACE_H_
#define ABEL_THREADING_INTERNAL_TSAN_MUTEX_INTERFACE_H_

// ABEL_INTERNAL_HAVE_TSAN_INTERFACE
// Macro intended only for internal use.
//
// Checks whether LLVM Thread Sanitizer interfaces are available.
// First made available in LLVM 5.0 (Sep 2017).
#ifdef ABEL_INTERNAL_HAVE_TSAN_INTERFACE
#error "ABEL_INTERNAL_HAVE_TSAN_INTERFACE cannot be directly set."
#endif

#if defined(THREAD_SANITIZER) && defined(__has_include)
#if __has_include(<sanitizer/tsan_interface.h>)
#define ABEL_INTERNAL_HAVE_TSAN_INTERFACE 1
#endif
#endif

#ifdef ABEL_INTERNAL_HAVE_TSAN_INTERFACE
#include <sanitizer/tsan_interface.h>

#define ABEL_TSAN_MUTEX_CREATE __tsan_mutex_create
#define ABEL_TSAN_MUTEX_DESTROY __tsan_mutex_destroy
#define ABEL_TSAN_MUTEX_PRE_LOCK __tsan_mutex_pre_lock
#define ABEL_TSAN_MUTEX_POST_LOCK __tsan_mutex_post_lock
#define ABEL_TSAN_MUTEX_PRE_UNLOCK __tsan_mutex_pre_unlock
#define ABEL_TSAN_MUTEX_POST_UNLOCK __tsan_mutex_post_unlock
#define ABEL_TSAN_MUTEX_PRE_SIGNAL __tsan_mutex_pre_signal
#define ABEL_TSAN_MUTEX_POST_SIGNAL __tsan_mutex_post_signal
#define ABEL_TSAN_MUTEX_PRE_DIVERT __tsan_mutex_pre_divert
#define ABEL_TSAN_MUTEX_POST_DIVERT __tsan_mutex_post_divert

#else

#define ABEL_TSAN_MUTEX_CREATE(...)
#define ABEL_TSAN_MUTEX_DESTROY(...)
#define ABEL_TSAN_MUTEX_PRE_LOCK(...)
#define ABEL_TSAN_MUTEX_POST_LOCK(...)
#define ABEL_TSAN_MUTEX_PRE_UNLOCK(...)
#define ABEL_TSAN_MUTEX_POST_UNLOCK(...)
#define ABEL_TSAN_MUTEX_PRE_SIGNAL(...)
#define ABEL_TSAN_MUTEX_POST_SIGNAL(...)
#define ABEL_TSAN_MUTEX_PRE_DIVERT(...)
#define ABEL_TSAN_MUTEX_POST_DIVERT(...)

#endif

#endif  // ABEL_THREADING_INTERNAL_TSAN_MUTEX_INTERFACE_H_
