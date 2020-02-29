
#ifndef ABEL_MATH_TRUNC_H_
#define ABEL_MATH_TRUNC_H_

#include <abel/math/option.h>

namespace abel {

    namespace math_internal {

        template<typename T>
        ABEL_CONSTEXPR T trunc_int(const T x) ABEL_NOEXCEPT {
            return (T(static_cast<long long>(x)));
        }

        template<typename T>
        ABEL_CONSTEXPR T trunc_check(const T x) ABEL_NOEXCEPT {
            return (
                    is_nan(x) ? std::numeric_limits<T>::quiet_NaN() : !is_finite(x) ?
                    x : std::numeric_limits<T>::epsilon() > abs(x) ?
                    x : trunc_int(x));
        }

    } //namespace math_internal


    template<typename T>
    ABEL_CONSTEXPR return_t <T> trunc(const T x) ABEL_NOEXCEPT {
        return math_internal::trunc_check(static_cast<return_t <T>>(x));
    }

} //namespace abel

#endif //ABEL_MATH_TRUNC_H_
