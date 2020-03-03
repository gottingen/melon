//
// Created by liyinbin on 2020/2/28.
//

#ifndef ABEL_MATH_ATAN2_H_
#define ABEL_MATH_ATAN2_H_

#include <abel/math/option.h>
#include <abel/math/is_nan.h>
#include <abel/math/neg_zero.h>
#include <abel/math/abs.h>
#include <abel/math/atan.h>

namespace abel {
    namespace math_internal {

        template<typename T>
        ABEL_CONSTEXPR T atan2_compute(const T y, const T x) ABEL_NOEXCEPT {
            return (any_nan(y, x) ? std::numeric_limits<T>::quiet_NaN() : std::numeric_limits<T>::epsilon() > abs(x) ?
                                                                          std::numeric_limits<T>::epsilon() > abs(y) ?
                                                                          neg_zero(y) ? neg_zero(x) ? -T(ABEL_PI) : -T(
                                                                                  0) : neg_zero(x) ? T(ABEL_PI) : T(0) :
                                                                          y > T(0) ? T(ABEL_HALF_PI) : -T(ABEL_HALF_PI)
                                                                                                                     :
                                                                          x < T(0) ? y < T(0) ?
                                                                                     atan(y / x) - T(ABEL_PI) :
                                                                                     atan(y / x) + T(ABEL_PI) : atan(
                                                                                  y / x));
        }

        template<typename T1, typename T2, typename TC = common_return_t<T1, T2>>
        ABEL_CONSTEXPR TC atan2_type_check(const T1 y, const T2 x) ABEL_NOEXCEPT {
            return atan2_compute(static_cast<TC>(x), static_cast<TC>(y));
        }

    } //namespace math_internal


    template<typename T1, typename T2>
    ABEL_CONSTEXPR common_return_t<T1, T2> atan2(const T1 y, const T2 x) ABEL_NOEXCEPT {
        return math_internal::atan2_type_check(x, y);
    }

} //namespace abel
#endif //ABEL_MATH_ATAN2_H_
