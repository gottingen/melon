//
// Created by liyinbin on 2019/12/8.
//

#ifndef ABEL_BASE_MATH_CLZ_H_
#define ABEL_BASE_MATH_CLZ_H_
#include <abel/base/profile.h>

namespace abel {

template<typename Integral>
ABEL_FORCE_INLINE unsigned clz_template (Integral x) {
    if (x == 0)
        return 8 * sizeof(x);
    unsigned r = 0;
    while ((x & (static_cast<Integral>(1) << (8 * sizeof(x) - 1))) == 0)
        x <<= 1, ++r;
    return r;
}

template<typename Integral>
ABEL_FORCE_INLINE unsigned clz (Integral x);

#if defined(ABEL_COMPILER_GNUC) || defined(ABEL_COMPILER_CLANG)

//! clz (count leading zeros)
template<>
ABEL_FORCE_INLINE unsigned clz<unsigned> (unsigned i) {
    if (i == 0)
        return 8 * sizeof(i);
    return static_cast<unsigned>(__builtin_clz(i));
}

//! clz (count leading zeros)
template<>
ABEL_FORCE_INLINE unsigned clz<int> (int i) {
    return clz(static_cast<unsigned>(i));
}

//! clz (count leading zeros)
template<>
ABEL_FORCE_INLINE unsigned clz<unsigned long> (unsigned long i) {
    if (i == 0)
        return 8 * sizeof(i);
    return static_cast<unsigned>(__builtin_clzl(i));
}

//! clz (count leading zeros)
template<>
ABEL_FORCE_INLINE unsigned clz<long> (long i) {
    return clz(static_cast<unsigned long>(i));
}

//! clz (count leading zeros)
template<>
ABEL_FORCE_INLINE unsigned clz<unsigned long long> (unsigned long long i) {
    if (i == 0)
        return 8 * sizeof(i);
    return static_cast<unsigned>(__builtin_clzll(i));
}

//! clz (count leading zeros)
template<>
ABEL_FORCE_INLINE unsigned clz<long long> (long long i) {
    return clz(static_cast<unsigned long long>(i));
}

#else

//! clz (count leading zeros)
template<>
ABEL_FORCE_INLINE unsigned clz<int> (int i) {
    return clz_template(i);
}

//! clz (count leading zeros)
template<>
ABEL_FORCE_INLINE unsigned clz<unsigned> (unsigned i) {
    return clz_template(i);
}

//! clz (count leading zeros)
template<>
ABEL_FORCE_INLINE unsigned clz<long> (long i) {
    return clz_template(i);
}

//! clz (count leading zeros)
template<>
ABEL_FORCE_INLINE unsigned clz<unsigned long> (unsigned long i) {
    return clz_template(i);
}

//! clz (count leading zeros)
template<>
ABEL_FORCE_INLINE unsigned clz<long long> (long long i) {
    return clz_template(i);
}

//! clz (count leading zeros)
template<>
ABEL_FORCE_INLINE unsigned clz<unsigned long long> (unsigned long long i) {
    return clz_template(i);
}

#endif

} //namespace abel

#endif //ABEL_BASE_MATH_CLZ_H_
