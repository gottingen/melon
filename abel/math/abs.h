
#ifndef ABEL_MATH_ABS_H_
#define ABEL_MATH_ABS_H_

#include <abel/math/option.h>

namespace abel {

    template<typename T>
    ABEL_DEPRECATED_MESSAGE("use std version instead")
    ABEL_CONSTEXPR T abs(const T x) ABEL_NOEXCEPT {
        return ( x == T(0) ? T(0) : x < T(0) ? -x : x);
    }
}
#endif //ABEL_MATH_ABS_H_
