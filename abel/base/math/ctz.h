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
ABEL_FORCE_INLINE unsigned count_tailing_zeros (Integral x);

//! count_tailing_zeros (count trailing zeros)
template<>
ABEL_FORCE_INLINE unsigned count_tailing_zeros<char> (char i) {
    return ctz_template(i);
}

//! count_tailing_zeros (count trailing zeros)
template<>
ABEL_FORCE_INLINE unsigned count_tailing_zeros<unsigned char> (unsigned char i) {
    return ctz_template(i);
}

#if defined(ABEL_COMPILER_GNUC) || defined(ABEL_COMPILER_CLANG)

//! count_tailing_zeros (count trailing zeros)
template<>
ABEL_FORCE_INLINE unsigned count_tailing_zeros<unsigned> (unsigned i) {
    if (i == 0)
        return 8 * sizeof(i);
    return static_cast<unsigned>(__builtin_ctz(i));
}

//! count_tailing_zeros (count trailing zeros)
template<>
ABEL_FORCE_INLINE unsigned count_tailing_zeros<int> (int i) {
    return count_tailing_zeros(static_cast<unsigned>(i));
}

//! count_tailing_zeros (count trailing zeros)
template<>
ABEL_FORCE_INLINE unsigned count_tailing_zeros<unsigned long> (unsigned long i) {
    if (i == 0)
        return 8 * sizeof(i);
    return static_cast<unsigned>(__builtin_ctzl(i));
}

//! count_tailing_zeros (count trailing zeros)
template<>
ABEL_FORCE_INLINE unsigned count_tailing_zeros<long> (long i) {
    return count_tailing_zeros(static_cast<unsigned long>(i));
}

//! count_tailing_zeros (count trailing zeros)
template<>
ABEL_FORCE_INLINE unsigned count_tailing_zeros<unsigned long long> (unsigned long long i) {
    if (i == 0)
        return 8 * sizeof(i);
    return static_cast<unsigned>(__builtin_ctzll(i));
}

//! count_tailing_zeros (count trailing zeros)
template<>
ABEL_FORCE_INLINE unsigned count_tailing_zeros<long long> (long long i) {
    return count_tailing_zeros(static_cast<unsigned long long>(i));
}

#else

//! count_tailing_zeros (count trailing zeros)
template<>
ABEL_FORCE_INLINE unsigned count_tailing_zeros<int> (int i) {
    return ctz_template(i);
}

//! count_tailing_zeros (count trailing zeros)
template<>
ABEL_FORCE_INLINE unsigned count_tailing_zeros<unsigned> (unsigned i) {
    return ctz_template(i);
}

//! count_tailing_zeros (count trailing zeros)
template<>
ABEL_FORCE_INLINE unsigned count_tailing_zeros<long> (long i) {
    return ctz_template(i);
}

//! count_tailing_zeros (count trailing zeros)
template<>
ABEL_FORCE_INLINE unsigned count_tailing_zeros<unsigned long> (unsigned long i) {
    return ctz_template(i);
}

//! count_tailing_zeros (count trailing zeros)
template<>
ABEL_FORCE_INLINE unsigned count_tailing_zeros<long long> (long long i) {
    return ctz_template(i);
}

//! count_tailing_zeros (count trailing zeros)
template<>
ABEL_FORCE_INLINE unsigned count_tailing_zeros<unsigned long long> (unsigned long long i) {
    return ctz_template(i);
}

#endif

} //namespace abel

#endif //ABEL_BASE_MATH_CTZ_H_
