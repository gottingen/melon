//
// Created by liyinbin on 2019/12/8.
//

#ifndef ABEL_BASE_MATH_FFS_H_
#define ABEL_BASE_MATH_FFS_H_

#include <abel/base/profile.h>

namespace abel {

//! ffs (find first set bit) - generic implementation
template<typename Integral>
static ABEL_FORCE_INLINE unsigned ffs_template (Integral x) {
    if (x == 0)
        return 0u;
    unsigned r = 1;
    while ((x & 1) == 0)
        x >>= 1, ++r;
    return r;
}

#if defined(ABEL_COMPILER_GNUC) || defined(ABEL_COMPILER_CLANG)

//! find first set bit in integer, or zero if none are set.
static ABEL_FORCE_INLINE
unsigned ffs(int i) {
    return static_cast<unsigned>(__builtin_ffs(i));
}

//! find first set bit in integer, or zero if none are set.
static ABEL_FORCE_INLINE
unsigned ffs(unsigned i) {
    return ffs(static_cast<int>(i));
}

//! find first set bit in integer, or zero if none are set.
static ABEL_FORCE_INLINE
unsigned ffs(long i) {
    return static_cast<unsigned>(__builtin_ffsl(i));
}

//! find first set bit in integer, or zero if none are set.
static ABEL_FORCE_INLINE
unsigned ffs(unsigned long i) {
    return ffs(static_cast<long>(i));
}

//! find first set bit in integer, or zero if none are set.
static ABEL_FORCE_INLINE
unsigned ffs(long long i) {
    return static_cast<unsigned>(__builtin_ffsll(i));
}

//! find first set bit in integer, or zero if none are set.
static ABEL_FORCE_INLINE
unsigned ffs(unsigned long long i) {
    return ffs(static_cast<long long>(i));
}

#else

//! find first set bit in integer, or zero if none are set.
static ABEL_FORCE_INLINE
unsigned ffs (int i) {
    return ffs_template(i);
}

//! find first set bit in integer, or zero if none are set.
static ABEL_FORCE_INLINE
unsigned ffs (unsigned int i) {
    return ffs_template(i);
}

//! find first set bit in integer, or zero if none are set.
static ABEL_FORCE_INLINE
unsigned ffs (long i) {
    return ffs_template(i);
}

//! find first set bit in integer, or zero if none are set.
static ABEL_FORCE_INLINE
unsigned ffs (unsigned long i) {
    return ffs_template(i);
}

//! find first set bit in integer, or zero if none are set.
static ABEL_FORCE_INLINE
unsigned ffs (long long i) {
    return ffs_template(i);
}

//! find first set bit in integer, or zero if none are set.
static ABEL_FORCE_INLINE
unsigned ffs (unsigned long long i) {
    return ffs_template(i);
}

#endif
} //namespace abel

#endif //ABEL_BASE_MATH_FFS_H_
