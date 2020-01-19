//
// Created by liyinbin on 2019/12/8.
//

#ifndef ABEL_BASE_MATH_CTZ_H_
#define ABEL_BASE_MATH_CTZ_H_
#include <abel/base/profile.h>

namespace abel {

/**
 * @brief generic implementation
 * @tparam Integral
 * @param x
 * @return
 */
template<typename Integral>
static ABEL_FORCE_INLINE unsigned ctz_template (Integral x) {
    if (x == 0)
        return 8 * sizeof(x);
    unsigned r = 0;
    while ((x & static_cast<Integral>(1)) == 0)
        x >>= 1, ++r;
    return r;
}

/******************************************************************************/

template<typename Integral>
ABEL_FORCE_INLINE unsigned ctz (Integral x);

#if defined(__GNUC__) || defined(__clang__)

//! ctz (count trailing zeros)
template <>
ABEL_FORCE_INLINE unsigned ctz<unsigned>(unsigned i) {
    if (i == 0) return 8 * sizeof(i);
    return static_cast<unsigned>(__builtin_ctz(i));
}

//! ctz (count trailing zeros)
template <>
ABEL_FORCE_INLINE unsigned ctz<int>(int i) {
    return ctz(static_cast<unsigned>(i));
}

//! ctz (count trailing zeros)
template <>
ABEL_FORCE_INLINE unsigned ctz<unsigned long>(unsigned long i) {
    if (i == 0) return 8 * sizeof(i);
    return static_cast<unsigned>(__builtin_ctzl(i));
}

//! ctz (count trailing zeros)
template <>
ABEL_FORCE_INLINE unsigned ctz<long>(long i) {
    return ctz(static_cast<unsigned long>(i));
}

//! ctz (count trailing zeros)
template <>
ABEL_FORCE_INLINE unsigned ctz<unsigned long long>(unsigned long long i) {
    if (i == 0) return 8 * sizeof(i);
    return static_cast<unsigned>(__builtin_ctzll(i));
}

//! ctz (count trailing zeros)
template <>
ABEL_FORCE_INLINE unsigned ctz<long long>(long long i) {
    return ctz(static_cast<unsigned long long>(i));
}

#else

//! ctz (count trailing zeros)
template<>
ABEL_FORCE_INLINE unsigned ctz<int> (int i) {
    return ctz_template(i);
}

//! ctz (count trailing zeros)
template<>
ABEL_FORCE_INLINE unsigned ctz<unsigned> (unsigned i) {
    return ctz_template(i);
}

//! ctz (count trailing zeros)
template<>
ABEL_FORCE_INLINE unsigned ctz<long> (long i) {
    return ctz_template(i);
}

//! ctz (count trailing zeros)
template<>
ABEL_FORCE_INLINE unsigned ctz<unsigned long> (unsigned long i) {
    return ctz_template(i);
}

//! ctz (count trailing zeros)
template<>
ABEL_FORCE_INLINE unsigned ctz<long long> (long long i) {
    return ctz_template(i);
}

//! ctz (count trailing zeros)
template<>
ABEL_FORCE_INLINE unsigned ctz<unsigned long long> (unsigned long long i) {
    return ctz_template(i);
}

#endif

} //namespace abel

#endif //ABEL_BASE_MATH_CTZ_H_
