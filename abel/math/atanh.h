//
// Created by liyinbin on 2020/2/28.
//

#ifndef ABEL_MATH_ATANH_H_
#define ABEL_MATH_ATANH_H_

#include <abel/math/option.h>
#include <abel/math/is_nan.h>
#include <abel/math/sgn.h>
#include <abel/math/abs.h>

namespace abel {

    namespace math_internal {

        template<typename T>
        ABEL_CONSTEXPR T atanh_compute(const T x) ABEL_NOEXCEPT {
            return (log((T(1) + x) / (T(1) - x)) / T(2));
        }

        template<typename T>
        ABEL_CONSTEXPR T atanh_check(const T x) ABEL_NOEXCEPT {
            return (is_nan(x) ? std::numeric_limits<T>::quiet_NaN() : T(1) < abs(x) ?
                                                                      std::numeric_limits<T>::quiet_NaN() :
                                                                      std::numeric_limits<T>::epsilon() >
                                                                      (T(1) - abs(x)) ?
                                                                      sgn(x) * std::numeric_limits<T>::infinity() :
                                                                      std::numeric_limits<T>::epsilon() > abs(x) ?
                                                                      T(0) : atanh_compute(x));
        }

    } //namespace math_internal


    template<typename T>
    ABEL_CONSTEXPR return_t<T> atanh(const T x) ABEL_NOEXCEPT {
        return math_internal::atanh_check(static_cast<return_t<T>>(x));
    }
}
#endif //ABEL_MATH_ATANH_H_
