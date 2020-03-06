

#ifndef ABEL_MATH_ACOS_H_
#define ABEL_MATH_ACOS_H_

#include <abel/math/option.h>
#include <abel/math/abs.h>
#include <abel/math/is_nan.h>
#include <abel/math/atan.h>

namespace abel {

    namespace math_internal {

        template<typename T>
        ABEL_CONSTEXPR T acos_compute(const T x) ABEL_NOEXCEPT {
            return (abel::abs(x) > T(1) ? std::numeric_limits<T>::quiet_NaN() :
                    std::numeric_limits<T>::epsilon() > abel::abs(x - T(1)) ?
                    T(0) : std::numeric_limits<T>::epsilon() > abel::abs(x) ?
                           T(ABEL_HALF_PI) : atan(sqrt(T(1) - x * x) / x)
            );
        }

        template<typename T>
        ABEL_CONSTEXPR T acos_check(const T x) ABEL_NOEXCEPT {
            return (is_nan(x) ? std::numeric_limits<T>::quiet_NaN() : x > T(0) ?
                                                                      acos_compute(x) : T(ABEL_PI) - acos_compute(-x)
            );
        }
    } //namespace math_internal

    template<typename T>
    ABEL_DEPRECATED_MESSAGE("use std version instead")
    ABEL_CONSTEXPR return_t<T> acos(const T x) ABEL_NOEXCEPT {
        return math_internal::acos_check(static_cast<return_t<T>>(x));
    }

}
#endif //ABEL_MATH_ACOS_H_
