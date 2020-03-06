//
// Created by liyinbin on 2020/2/28.
//

#ifndef ABEL_MATH_LCM_H_
#define ABEL_MATH_LCM_H_

#include <abel/math/option.h>
#include <abel/math/gcd.h>

namespace abel {

    namespace math_internal {

        template<typename T>
        ABEL_CONSTEXPR T lcm_compute(const T a, const T b) ABEL_NOEXCEPT {
            return abs(a * (b / gcd(a, b)));
        }

        template<typename T1, typename T2, typename TC = common_t<T1, T2>>
        ABEL_CONSTEXPR TC lcm_type_check(const T1 a, const T2 b) ABEL_NOEXCEPT {
            return lcm_compute(static_cast<TC>(a), static_cast<TC>(b));
        }

    } //namespace math_internal



    template<typename T1, typename T2>
    ABEL_DEPRECATED_MESSAGE("use std version instead")
    ABEL_CONSTEXPR common_t<T1, T2> lcm(const T1 a, const T2 b) ABEL_NOEXCEPT {
        return math_internal::lcm_type_check(a, b);
    }
}
#endif //ABEL_MATH_LCM_H_
