//
// Created by liyinbin on 2019/12/8.
//

#ifndef ABEL_BASE_MATH_POWER2_H_
#define ABEL_BASE_MATH_POWER2_H_

#include <abel/base/profile.h>

namespace abel {

template<typename Integral>
static ABEL_FORCE_INLINE bool is_power_of_two_template (Integral i) {
    if (i <= 0)
        return false;
    return !(i & (i - 1));
}

// is_power_of_two()

//! does what it says: true if i is a power of two
static ABEL_FORCE_INLINE bool is_power_of_two (int i) {
    return is_power_of_two_template(i);
}

//! does what it says: true if i is a power of two
static ABEL_FORCE_INLINE bool is_power_of_two (unsigned int i) {
    return is_power_of_two_template(i);
}

//! does what it says: true if i is a power of two
static ABEL_FORCE_INLINE bool is_power_of_two (long i) {
    return is_power_of_two_template(i);
}

//! does what it says: true if i is a power of two
static ABEL_FORCE_INLINE bool is_power_of_two (unsigned long i) {
    return is_power_of_two_template(i);
}

//! does what it says: true if i is a power of two
static ABEL_FORCE_INLINE bool is_power_of_two (long long i) {
    return is_power_of_two_template(i);
}

//! does what it says: true if i is a power of two
static ABEL_FORCE_INLINE bool is_power_of_two (unsigned long long i) {
    return is_power_of_two_template(i);
}

} //namespace abel

#endif //ABEL_BASE_MATH_POWER2_H_
