//
// Created by liyinbin on 2019/12/8.
//

#ifndef ABEL_BASE_MATH_POWER_H_
#define ABEL_BASE_MATH_POWER_H_

#include <abel/base/math/round.h>

namespace abel {

    template<unsigned D, typename T>
    static ABEL_FORCE_INLINE constexpr
    T power_to_the(T x) {
        // Compiler optimize two calls to the same recursion into one
        // Tested with GCC 4+, Clang 3+, MSVC 15+
        return D < 1 ? 1 : D == 1 ? x
                                  : power_to_the<D / 2>(x) * power_to_the<div_ceil(D, 2)>(x);
    }

} //namespace abel
#endif //ABEL_BASE_MATH_POWER_H_
