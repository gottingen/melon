//
// Created by liyinbin on 2020/2/28.
//

#ifndef ABEL_EXP_H
#define ABEL_EXP_H

#include <abel/math/option.h>
#include <abel/math/is_nan.h>
#include <abel/math/is_inf.h>
#include <abel/math/find_whole.h>
#include <abel/math/pow_integer.h>
#include <abel/math/find_fraction.h>

namespace abel {

    namespace math_internal {

        template<typename T>
        ABEL_CONSTEXPR T exp_cf_recur(const T x, const int depth) ABEL_NOEXCEPT {
            return (depth < ABEL_EXP_MAX_ITER_SMALL ? depth == 1 ? T(1) - x / exp_cf_recur(x, depth + 1) :
                                                      T(1) + x / T(depth - 1) - x / depth / exp_cf_recur(x, depth + 1)
                                                    : T(1));
        }

        template<typename T>
        ABEL_CONSTEXPR T exp_cf(const T x) ABEL_NOEXCEPT {
            return (T(1) / exp_cf_recur(x, 1));
        }

        template<typename T>
        ABEL_CONSTEXPR T exp_split(const T x) ABEL_NOEXCEPT {
            return (pow_integral(ABEL_E, find_whole(x)) * exp_cf(find_fraction(x)));
        }

        template<typename T>
        ABEL_CONSTEXPR T exp_check(const T x) ABEL_NOEXCEPT {
            return (is_nan(x) ? std::numeric_limits<T>::quiet_NaN() : is_neginf(x) ?
                                                                      T(0) : std::numeric_limits<T>::epsilon() > abs(x)
                                                                             ?
                                                                             T(1) : is_posinf(x) ?
                                                                                    std::numeric_limits<T>::infinity() :
                                                                                    abs(x) < T(2) ?
                                                                                    exp_cf(x) : exp_split(x));
        }

    } //namespace math_internal


    template<typename T>
    ABEL_CONSTEXPR return_t<T> exp(const T x) ABEL_NOEXCEPT {
        return math_internal::exp_check(static_cast<return_t<T>>(x));
    }
}
#endif //ABEL_EXP_H
