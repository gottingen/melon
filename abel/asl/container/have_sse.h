//
// Shared config probing for SSE instructions used in Swiss tables.
#ifndef ABEL_ASL_CONTAINER_HAVE_SSE_H_
#define ABEL_ASL_CONTAINER_HAVE_SSE_H_

#ifndef SWISSTABLE_HAVE_SSE2
#if defined(__SSE2__) || \
    (defined(_MSC_VER) && \
     (defined(_M_X64) || (defined(_M_IX86) && _M_IX86_FP >= 2)))
#define SWISSTABLE_HAVE_SSE2 1
#else
#define SWISSTABLE_HAVE_SSE2 0
#endif
#endif

#ifndef SWISSTABLE_HAVE_SSSE3
#ifdef __SSSE3__
#define SWISSTABLE_HAVE_SSSE3 1
#else
#define SWISSTABLE_HAVE_SSSE3 0
#endif
#endif

#if SWISSTABLE_HAVE_SSSE3 && !SWISSTABLE_HAVE_SSE2
#error "Bad configuration!"
#endif

#if SWISSTABLE_HAVE_SSE2

#include <emmintrin.h>

#endif

#if SWISSTABLE_HAVE_SSSE3

#include <tmmintrin.h>

#endif

#endif  // ABEL_ASL_CONTAINER_HAVE_SSE_H_
