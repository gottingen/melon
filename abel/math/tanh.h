
#ifndef ABEL_MATH_TANH_H_
#define ABEL_MATH_TANH_H_

#include <abel/math/option.h>
#include <abel/math/abs.h>
#include <abel/math/pow_integer.h>
#include <abel/math/is_nan.h>

namespace abel {
    namespace math_internal {

        template<typename T>
        ABEL_CONSTEXPR T tanh_cf(const T xx, const int depth) ABEL_NOEXCEPT {
            return (depth < ABEL_TANH_MAX_ITER ? (2 * depth - 1) + xx / tanh_cf(xx, depth + 1) : T(2 * depth - 1));
        }

        template<typename T>
        ABEL_CONSTEXPR T tanh_begin(const T x) ABEL_NOEXCEPT {
            return (x / tanh_cf(x * x, 1));
        }

        template<typename T>
        ABEL_CONSTEXPR T tanh_check(const T x) ABEL_NOEXCEPT {
            return ( is_nan(x) ? std::numeric_limits<T>::quiet_NaN() : std::numeric_limits<T>::epsilon() > abs(x) ?
                T(0) : x < T(0) ? -tanh_begin(-x) :
                    tanh_begin(x)
            );
        }
    } //namespace math_internal

    template<typename T>
    ABEL_CONSTEXPR return_t<T> tanh(const T x) ABEL_NOEXCEPT {
        return math_internal::tanh_check(static_cast<return_t<T>>(x));
    }

} // namespace abel
#endif //ABEL_MATH_TANH_H_
