//
// Created by liyinbin on 2020/2/28.
//

#ifndef ABEL_MATH_ERF_H_
#define ABEL_MATH_ERF_H_

#include <abel/math/option.h>
#include <abel/math/is_nan.h>
#include <abel/math/is_inf.h>
#include <abel/math/abs.h>

namespace abel {

    namespace math_internal {

        // see
        // http://functions.wolfram.com/GammaBetaErf/Erf/10/01/0007/

        template<typename T>
        ABEL_CONSTEXPR T erf_cf_large_recur(const T x, const int depth) ABEL_NOEXCEPT {
            return (depth < ABEL_ERF_MAX_ITER ? x + 2 * depth / erf_cf_large_recur(x, depth + 1) : x);
        }

        template<typename T>
        ABEL_CONSTEXPR T erf_cf_large_main(const T x) ABEL_NOEXCEPT {
            return (T(1) - T(2) * (exp(-x * x) / T(ABEL_SQRT_PI)) / erf_cf_large_recur(T(2) * x, 1));
        }

        // see
        // http://functions.wolfram.com/GammaBetaErf/Erf/10/01/0005/

        template<typename T>
        ABEL_CONSTEXPR T erf_cf_small_recur(const T xx, const int depth) ABEL_NOEXCEPT {
            return (depth < ABEL_ERF_MAX_ITER ? (2 * depth - 1) - 2 * xx +
                                                4 * depth * xx / erf_cf_small_recur(xx, depth + 1) :
                    (2 * depth - 1) - 2 * xx);
        }

        template<typename T>
        ABEL_CONSTEXPR T erf_cf_small_main(const T x) ABEL_NOEXCEPT {
            return (T(2) * x * (exp(-x * x) / T(ABEL_SQRT_PI)) / erf_cf_small_recur(x * x, 1));
        }

        template<typename T>
        ABEL_CONSTEXPR T erf_begin(const T x) ABEL_NOEXCEPT {
            return (x > T(2.1) ? erf_cf_large_main(x) : erf_cf_small_main(x));
        }

        template<typename T>
        ABEL_CONSTEXPR T erf_check(const T x) ABEL_NOEXCEPT {
            return (is_nan(x) ? std::numeric_limits<T>::quiet_NaN() : is_posinf(x) ? T(1) : is_neginf(x) ? -T(1) :
                                                                                            std::numeric_limits<T>::epsilon() >
                                                                                            abs(x) ? T(0) : x < T(0)
                                                                                                            ? -erf_begin(
                                                                                                            -x)
                                                                                                            : erf_begin(
                                                                                                            x));
        }

    } //namespace math_internal


    template<typename T>
    ABEL_DEPRECATED_MESSAGE("use std version instead")
    ABEL_CONSTEXPR return_t<T> erf(const T x) ABEL_NOEXCEPT {
        return math_internal::erf_check(static_cast<return_t<T>>(x));
    }

}
#endif //ABEL_MATH_ERF_H_
