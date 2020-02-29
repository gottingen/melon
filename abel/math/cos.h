//
// Created by liyinbin on 2020/2/28.
//

#ifndef ABEL_MATH_COS_H_
#define ABEL_MATH_COS_H_

#include <abel/math/option.h>
#include <abel/math/is_nan.h>

namespace abel {

    namespace math_internal {

        template<typename T>
        ABEL_CONSTEXPR T cos_compute(const T x) ABEL_NOEXCEPT {
            return (T(1) - x * x) / (T(1) + x * x);
        }

        template<typename T>
        ABEL_CONSTEXPR T cos_check(const T x) ABEL_NOEXCEPT {
            return (is_nan(x) ? std::numeric_limits<T>::quiet_NaN() :
                    std::numeric_limits<T>::epsilon() > abs(x) ? T(1) :
                    std::numeric_limits<T>::epsilon() > abs(x - T(ABEL_HALF_PI)) ? T(0) :
                    std::numeric_limits<T>::epsilon() > abs(x + T(ABEL_HALF_PI)) ? T(0) :
                    std::numeric_limits<T>::epsilon() > abs(x - T(ABEL_PI)) ? -T(1) :
                    std::numeric_limits<T>::epsilon() > abs(x + T(ABEL_PI)) ? -T(1) :
                    cos_compute(tan(x / T(2))));
        }

    } //namespace math_internal


    template<typename T>
    ABEL_CONSTEXPR return_t<T> cos(const T x) ABEL_NOEXCEPT {
        return math_internal::cos_check(static_cast<return_t<T>>(x));
    }

} //namespace abel

#endif //ABEL_MATH_COS_H_
