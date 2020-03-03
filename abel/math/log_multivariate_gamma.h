//
// Created by liyinbin on 2020/2/28.
//

#ifndef ABEL_MATH_LOG_MULTIVARIATE_GAMMA_H_
#define ABEL_MATH_LOG_MULTIVARIATE_GAMMA_H_

#include <abel/math/option.h>

namespace abel {

    namespace math_internal {

        // see https://en.wikipedia.org/wiki/Multivariate_gamma_function

        template<typename T1, typename T2>
        ABEL_CONSTEXPR T1 lmgamma_recur(const T1 a, const T2 p) ABEL_NOEXCEPT {
            return (is_nan(a) ? std::numeric_limits<T1>::quiet_NaN() :
                    p == T2(1) ? lgamma(a) :
                    p < T2(1) ? std::numeric_limits<T1>::quiet_NaN() :
                    T1(ABEL_LOG_PI) * (p - T1(1)) / T1(2) + lgamma(a) + lmgamma_recur(a - T1(0.5), p - T2(1)));
        }

    } //namespace math_internal


    template<typename T1, typename T2>
    ABEL_CONSTEXPR return_t<T1> lmgamma(const T1 a, const T2 p) ABEL_NOEXCEPT {
        return math_internal::lmgamma_recur(static_cast<return_t<T1>>(a), p);
    }

}
#endif //ABEL_MATH_LOG_MULTIVARIATE_GAMMA_H_
