//
// Created by liyinbin on 2020/2/28.
//

#ifndef ABEL_ROUND_H
#define ABEL_ROUND_H

#include <abel/math/option.h>
#include <abel/math/is_nan.h>
#include <abel/math/is_finit.h>
#include <abel/math/sgn.h>
#include <abel/math/find_whole.h>

namespace abel {

    namespace math_internal {

        template<typename T>
        ABEL_CONSTEXPR T round_int(const T x) ABEL_NOEXCEPT {
            return static_cast<T>(find_whole(x));
        }

        template<typename T>
        ABEL_CONSTEXPR T round_check(const T x) ABEL_NOEXCEPT {
            return (is_nan(x) ? std::numeric_limits<T>::quiet_NaN() : !is_finite(x) ? x :
                                                                      std::numeric_limits<T>::epsilon() > abs(x) ?
                                                                      x : sgn(x) * round_int(abs(x)));
        }

    } //namespace math_internal


    template<typename T>
    ABEL_CONSTEXPR return_t<T> round(const T x) ABEL_NOEXCEPT {
        return math_internal::round_check(static_cast<return_t<T>>(x));
    }


//! calculate n div k with rounding up, for n and k positive!
    template<typename IntegralN, typename IntegralK>
    ABEL_CONSTEXPR
    auto div_ceil(const IntegralN &n, const IntegralK &k) -> decltype(n + k) {
        return (n + k - 1) / k;
    }


    template<typename Integral>
    static ABEL_FORCE_INLINE Integral pow2_ceil_template(Integral n) {
        --n;
        for (size_t k = 1; k != 8 * sizeof(n); k <<= 1) {
            n |= n >> k;
        }
        ++n;
        return n;
    }

// pow2_ceil()

//! does what it says: round up to next power of two
    static ABEL_FORCE_INLINE int pow2_ceil(int i) {
        return pow2_ceil_template(i);
    }

//! does what it says: round up to next power of two
    static ABEL_FORCE_INLINE unsigned int pow2_ceil(unsigned int i) {
        return pow2_ceil_template(i);
    }

//! does what it says: round up to next power of two
    static ABEL_FORCE_INLINE long pow2_ceil(long i) {
        return pow2_ceil_template(i);
    }

//! does what it says: round up to next power of two
    static ABEL_FORCE_INLINE unsigned long pow2_ceil(unsigned long i) {
        return pow2_ceil_template(i);
    }

//! does what it says: round up to next power of two
    static ABEL_FORCE_INLINE long long pow2_ceil(long long i) {
        return pow2_ceil_template(i);
    }

//! does what it says: round up to next power of two
    static ABEL_FORCE_INLINE
    unsigned long long pow2_ceil(unsigned long long i) {
        return pow2_ceil_template(i);
    }

// pow2_floor()

//! does what it says: round down to next power of two
    static ABEL_FORCE_INLINE int pow2_floor(int i) {
        return pow2_ceil(i + 1) >> 1;
    }

//! does what it says: round down to next power of two
    static ABEL_FORCE_INLINE unsigned int pow2_floor(unsigned int i) {
        return pow2_ceil(i + 1) >> 1;
    }

//! does what it says: round down to next power of two
    static ABEL_FORCE_INLINE long pow2_floor(long i) {
        return pow2_ceil(i + 1) >> 1;
    }

//! does what it says: round down to next power of two
    static ABEL_FORCE_INLINE unsigned long pow2_floor(unsigned long i) {
        return pow2_ceil(i + 1) >> 1;
    }

//! does what it says: round down to next power of two
    static ABEL_FORCE_INLINE long long pow2_floor(long long i) {
        return pow2_ceil(i + 1) >> 1;
    }

//! does what it says: round down to next power of two
    static ABEL_FORCE_INLINE
    unsigned long long pow2_floor(unsigned long long i) {
        return pow2_ceil(i + 1) >> 1;
    }

    template<typename IntegralN, typename IntegralK>
    ABEL_CONSTEXPR
    auto round_up(const IntegralN &n, const IntegralK &k) -> decltype(n + k) {
        return ((n + k - 1) / k) * k;
    }


}
#endif //ABEL_ROUND_H
