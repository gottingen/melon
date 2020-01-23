
#ifndef ABEL_THREADING_INTERNAL_PER_THREAD_TLS_H_
#define ABEL_THREADING_INTERNAL_PER_THREAD_TLS_H_

// This header defines two macros:
//
// If the platform supports thread-local storage:
//
// * ABEL_PER_THREAD_TLS_KEYWORD is the C keyword needed to declare a
//   thread-local variable
// * ABEL_PER_THREAD_TLS is 1
//
// Otherwise:
//
// * ABEL_PER_THREAD_TLS_KEYWORD is empty
// * ABEL_PER_THREAD_TLS is 0
//
// Microsoft C supports thread-local storage.
// GCC supports it if the appropriate version of glibc is available,
// which the programmer can indicate by defining ABEL_HAVE_TLS

#include <abel/base/profile.h>  // For ABEL_HAVE_TLS

#if defined(ABEL_PER_THREAD_TLS)
#error ABEL_PER_THREAD_TLS cannot be directly set
#elif defined(ABEL_PER_THREAD_TLS_KEYWORD)
#error ABEL_PER_THREAD_TLS_KEYWORD cannot be directly set
#elif defined(ABEL_HAVE_TLS)
#define ABEL_PER_THREAD_TLS_KEYWORD __thread
#define ABEL_PER_THREAD_TLS 1
#elif defined(_MSC_VER)
#define ABEL_PER_THREAD_TLS_KEYWORD __declspec(thread)
#define ABEL_PER_THREAD_TLS 1
#else
#define ABEL_PER_THREAD_TLS_KEYWORD
#define ABEL_PER_THREAD_TLS 0
#endif

#endif  // ABEL_THREADING_INTERNAL_PER_THREAD_TLS_H_
