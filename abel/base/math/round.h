//
// Created by liyinbin on 2019/12/8.
//

#ifndef ABEL_BASE_MATH_ROUND_H_
#define ABEL_BASE_MATH_ROUND_H_

#include <abel/base/profile.h>

namespace abel {

//! calculate n div k with rounding up, for n and k positive!
template<typename IntegralN, typename IntegralK>
static ABEL_FORCE_INLINE constexpr
auto div_ceil (const IntegralN &n, const IntegralK &k) -> decltype(n + k) {
    return (n + k - 1) / k;
}

// round_up_to_power_of_two()

template<typename Integral>
static ABEL_FORCE_INLINE Integral round_up_to_power_of_two_template (Integral n) {
    --n;
    for (size_t k = 1; k != 8 * sizeof(n); k <<= 1) {
        n |= n >> k;
    }
    ++n;
    return n;
}

// round_up_to_power_of_two()

//! does what it says: round up to next power of two
static ABEL_FORCE_INLINE int round_up_to_power_of_two (int i) {
    return round_up_to_power_of_two_template(i);
}

//! does what it says: round up to next power of two
static ABEL_FORCE_INLINE unsigned int round_up_to_power_of_two (unsigned int i) {
    return round_up_to_power_of_two_template(i);
}

//! does what it says: round up to next power of two
static ABEL_FORCE_INLINE long round_up_to_power_of_two (long i) {
    return round_up_to_power_of_two_template(i);
}

//! does what it says: round up to next power of two
static ABEL_FORCE_INLINE unsigned long round_up_to_power_of_two (unsigned long i) {
    return round_up_to_power_of_two_template(i);
}

//! does what it says: round up to next power of two
static ABEL_FORCE_INLINE long long round_up_to_power_of_two (long long i) {
    return round_up_to_power_of_two_template(i);
}

//! does what it says: round up to next power of two
static ABEL_FORCE_INLINE
unsigned long long round_up_to_power_of_two (unsigned long long i) {
    return round_up_to_power_of_two_template(i);
}

// round_down_to_power_of_two()

//! does what it says: round down to next power of two
static ABEL_FORCE_INLINE int round_down_to_power_of_two (int i) {
    return round_up_to_power_of_two(i + 1) >> 1;
}

//! does what it says: round down to next power of two
static ABEL_FORCE_INLINE unsigned int round_down_to_power_of_two (unsigned int i) {
    return round_up_to_power_of_two(i + 1) >> 1;
}

//! does what it says: round down to next power of two
static ABEL_FORCE_INLINE long round_down_to_power_of_two (long i) {
    return round_up_to_power_of_two(i + 1) >> 1;
}

//! does what it says: round down to next power of two
static ABEL_FORCE_INLINE unsigned long round_down_to_power_of_two (unsigned long i) {
    return round_up_to_power_of_two(i + 1) >> 1;
}

//! does what it says: round down to next power of two
static ABEL_FORCE_INLINE long long round_down_to_power_of_two (long long i) {
    return round_up_to_power_of_two(i + 1) >> 1;
}

//! does what it says: round down to next power of two
static ABEL_FORCE_INLINE
unsigned long long round_down_to_power_of_two (unsigned long long i) {
    return round_up_to_power_of_two(i + 1) >> 1;
}

template<typename IntegralN, typename IntegralK>
static ABEL_FORCE_INLINE constexpr
auto round_up (const IntegralN &n, const IntegralK &k) -> decltype(n + k) {
    return ((n + k - 1) / k) * k;
}

} //namespace abel

#endif //ABEL_BASE_MATH_ROUND_H_
