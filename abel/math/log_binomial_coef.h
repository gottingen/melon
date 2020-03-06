//
// Created by liyinbin on 2020/2/28.
//

#ifndef ABEL_MATH_LOG_BINOMIAL_COEF_H_
#define ABEL_MATH_LOG_BINOMIAL_COEF_H_

#include <abel/math/option.h>

namespace abel {

    namespace math_internal {

        template<typename T>
        ABEL_CONSTEXPR T log_binomial_coef_compute(const T n, const T k) ABEL_NOEXCEPT {
            return (lgamma(n + 1) - (lgamma(k + 1) + lgamma(n - k + 1)));
        }

        template<typename T1, typename T2, typename TC = common_return_t<T1, T2>>
        ABEL_CONSTEXPR TC log_binomial_coef_type_check(const T1 n, const T2 k) ABEL_NOEXCEPT {
            return log_binomial_coef_compute(static_cast<TC>(n), static_cast<TC>(k));
        }

    } //namespace math_internal


    template<typename T1, typename T2>
    ABEL_DEPRECATED_MESSAGE("use std version instead")
    ABEL_CONSTEXPR common_return_t<T1, T2> log_binomial_coef(const T1 n, const T2 k) ABEL_NOEXCEPT {
        return math_internal::log_binomial_coef_type_check(n, k);
    }

} //namespace abel

#endif //ABEL_MATH_LOG_BINOMIAL_COEF_H_
