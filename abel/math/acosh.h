

#ifndef ABEL_MATH_ACOSH_H_
#define ABEL_MATH_ACOSH_H_

#include <abel/math/option.h>
#include <abel/math/abs.h>
#include <abel/math/is_nan.h>
#include <abel/math/log.h>

namespace abel {
    namespace math_internal {


        template<typename T>
        ABEL_CONSTEXPR T acosh_compute(const T x) ABEL_NOEXCEPT {
            return (is_nan(x) ? std::numeric_limits<T>::quiet_NaN() : x < T(1) ?
                                                                      std::numeric_limits<T>::quiet_NaN() :
                                                                      std::numeric_limits<T>::epsilon() > abs(x - T(1))
                                                                      ?
                                                                      T(0) : log(x + sqrt(x * x - T(1)))
            );
        }
    } //namespace math_internal

    template<typename T>
    ABEL_CONSTEXPR return_t<T> acosh(const T x) ABEL_NOEXCEPT {
        return math_internal::acosh_compute(static_cast<return_t<T>>(x));
    }

} //namespace abel

#endif //ABEL_MATH_ACOSH_H_
