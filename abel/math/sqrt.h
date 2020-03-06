
#ifndef ABEL_MATH_SQRT_H_
#define ABEL_MATH_SQRT_H_

#include <abel/math/option.h>
#include <abel/math/abs.h>
#include <abel/math/is_nan.h>
#include <abel/math/is_inf.h>

namespace abel {
    namespace math_internal {
        template<typename T>
        ABEL_CONSTEXPR T sqrt_recur(const T x, const T xn, const int count) ABEL_NOEXCEPT {
            return (abel::abs(xn - x / xn) / (T(1) + xn) < std::numeric_limits<T>::epsilon() ? xn : count < ABEL_SQRT_MAX_ITER ?
                sqrt_recur(x, T(0.5) * (xn + x / xn), count + 1) : xn
                );
        }

        template<typename T>
        ABEL_CONSTEXPR T sqrt_check(const T x, const T m_val) ABEL_NOEXCEPT {
            return (is_nan(x) ? std::numeric_limits<T>::quiet_NaN() : x < T(0) ?
                std::numeric_limits<T>::quiet_NaN() : is_posinf(x) ?
                x : std::numeric_limits<T>::epsilon() > abel::abs(x) ?
                    T(0) : std::numeric_limits<T>::epsilon() > abel::abs(T(1) - x) ?
                        x : x > T(4) ?
                            sqrt_check(x / T(4), T(2) * m_val) : m_val * sqrt_recur(x, x / T(2), 0)
                        );
        }

    }

    template<typename T>
    ABEL_DEPRECATED_MESSAGE("use std version instead")
    ABEL_CONSTEXPR return_t<T> sqrt(const T x) ABEL_NOEXCEPT {
        return math_internal::sqrt_check(static_cast<return_t<T>>(x), return_t<T>(1));
    }

} //namespase abel

#endif //ABEL_MATH_SQRT_H_
