//
// Created by liyinbin on 2020/3/1.
//

#ifndef ABEL_POW_TRAITS_H
#define ABEL_POW_TRAITS_H

#include <abel/base/profile.h>


namespace abel {

    template<typename Integral>
    static ABEL_FORCE_INLINE bool is_power_of_two_template(Integral i) {
        if (i <= 0)
            return false;
        return !(i & (i - 1));
    }

// is_power_of_two()

//! does what it says: true if i is a power of two
    static ABEL_FORCE_INLINE bool is_power_of_two(int i) {
        return is_power_of_two_template(i);
    }

//! does what it says: true if i is a power of two
    static ABEL_FORCE_INLINE bool is_power_of_two(unsigned int i) {
        return is_power_of_two_template(i);
    }

//! does what it says: true if i is a power of two
    static ABEL_FORCE_INLINE bool is_power_of_two(long i) {
        return is_power_of_two_template(i);
    }

//! does what it says: true if i is a power of two
    static ABEL_FORCE_INLINE bool is_power_of_two(unsigned long i) {
        return is_power_of_two_template(i);
    }

//! does what it says: true if i is a power of two
    static ABEL_FORCE_INLINE bool is_power_of_two(long long i) {
        return is_power_of_two_template(i);
    }

//! does what it says: true if i is a power of two
    static ABEL_FORCE_INLINE bool is_power_of_two(unsigned long long i) {
        return is_power_of_two_template(i);
    }
}
#endif //ABEL_POW_TRAITS_H
