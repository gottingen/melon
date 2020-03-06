//
// Created by liyinbin on 2020/2/28.
//

#ifndef ABEL_MATH_ATAN_H_
#define ABEL_MATH_ATAN_H_

#include <abel/math/option.h>
#include <abel/math/is_nan.h>
#include <abel/math/pow.h>
#include <abel/math/abs.h>

namespace abel {

    namespace math_internal {

        template<typename T>
        ABEL_CONSTEXPR T atan_series_order_calc(const T x, const T x_pow, const unsigned int order) ABEL_NOEXCEPT {
            return (T(1) / (T((order - 1) * 4 - 1) * x_pow) - T(1) / (T((order - 1) * 4 + 1) * x_pow * x));
        }

        template<typename T>
        ABEL_CONSTEXPR T atan_series_order(const T x, const T x_pow, const unsigned int order,
                          const unsigned int max_order) ABEL_NOEXCEPT {
            return (order == 1 ? ABEL_HALF_PI - T(1) / x + atan_series_order(x * x, pow(x, 3), order + 1, max_order) :
                    order < max_order ? atan_series_order_calc(x, x_pow, order) +
                                        atan_series_order(x, x_pow * x * x, order + 1, max_order) :
                    atan_series_order_calc(x, x_pow, order));
        }

        template<typename T>
        ABEL_CONSTEXPR T atan_series_main(const T x) ABEL_NOEXCEPT {
            return (x < T(3) ? atan_series_order(x, x, 1U, 10U) :  // O(1/x^39)
                    x < T(4) ? atan_series_order(x, x, 1U, 9U) :  // O(1/x^35)
                    x < T(5) ? atan_series_order(x, x, 1U, 8U) :  // O(1/x^31)
                    x < T(7) ? atan_series_order(x, x, 1U, 7U) :  // O(1/x^27)
                    x < T(11) ? atan_series_order(x, x, 1U, 6U) :  // O(1/x^23)
                    x < T(25) ? atan_series_order(x, x, 1U, 5U) :  // O(1/x^19)
                    x < T(100) ? atan_series_order(x, x, 1U, 4U) :  // O(1/x^15)
                    x < T(1000) ? atan_series_order(x, x, 1U, 3U) :  // O(1/x^11)
                    atan_series_order(x, x, 1U, 2U));  // O(1/x^7)
        }


        template<typename T>
        ABEL_CONSTEXPR T
        atan_cf_recur(const T xx, const unsigned int depth, const unsigned int max_depth) ABEL_NOEXCEPT {
            return (depth < max_depth ? T(2 * depth - 1) + depth * depth * xx / atan_cf_recur(xx, depth + 1, max_depth)
                                      :
                    T(2 * depth - 1));
        }

        template<typename T>
        ABEL_CONSTEXPR T atan_cf_main(const T x) ABEL_NOEXCEPT {
            return (x < T(0.5) ? x / atan_cf_recur(x * x, 1U, 15U) :
                    x < T(1) ? x / atan_cf_recur(x * x, 1U, 25U) :
                    x < T(1.5) ? x / atan_cf_recur(x * x, 1U, 35U) :
                    x < T(2) ? x / atan_cf_recur(x * x, 1U, 45U) :
                    x / atan_cf_recur(x * x, 1U, 52U));
        }


        template<typename T>
        ABEL_CONSTEXPR T atan_begin(const T x) ABEL_NOEXCEPT {
            return (x > T(2.5) ? atan_series_main(x) : atan_cf_main(x));
        }

        template<typename T>
        ABEL_CONSTEXPR T atan_check(const T x) ABEL_NOEXCEPT {
            return (is_nan(x) ? std::numeric_limits<T>::quiet_NaN() : std::numeric_limits<T>::epsilon() > abs(x) ? T(0)
                                                                                                                 :
                                                                      x < T(0) ? -atan_begin(-x) : atan_begin(x));
        }

    } //namespace math_internal


    template<typename T>
    ABEL_DEPRECATED_MESSAGE("use std version instead")
    ABEL_CONSTEXPR return_t<T> atan(const T x) ABEL_NOEXCEPT {
        return math_internal::atan_check(static_cast<return_t<T>>(x));
    }

}
#endif //ABEL_MATH_ATAN_H_
