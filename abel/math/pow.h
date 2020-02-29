//
// Created by liyinbin on 2020/2/28.
//

#ifndef ABEL_POW_H
#define ABEL_POW_H

#include <abel/math/option.h>
#include <abel/math/pow_integer.h>

namespace abel {

    namespace math_internal {

        template<typename T>
        ABEL_CONSTEXPR T pow_dbl(const T base, const T exp_term) ABEL_NOEXCEPT {
            return exp(exp_term * log(base));
        }

        template<typename T1, typename T2, typename TC = common_t<T1, T2>,
                typename std::enable_if<!std::is_integral<T2>::value>::type * = nullptr>
        ABEL_CONSTEXPR TC pow_check(const T1 base, const T2 exp_term) ABEL_NOEXCEPT {
            return (base < T1(0) ?
                std::numeric_limits<TC>::quiet_NaN() : pow_dbl(static_cast<TC>(base), static_cast<TC>(exp_term)));
        }

        template<typename T1, typename T2, typename TC = common_t<T1, T2>,
                typename std::enable_if<std::is_integral<T2>::value>::type * = nullptr>
        ABEL_CONSTEXPR TC pow_check(const T1 base, const T2 exp_term) ABEL_NOEXCEPT {
            return pow_integral(base, exp_term);
        }

    } //namespace math_internal


    template<typename T1, typename T2>
    ABEL_CONSTEXPR common_t<T1, T2> pow(const T1 base, const T2 exp_term) ABEL_NOEXCEPT {
        return math_internal::pow_check(base, exp_term);
    }

}
#endif //ABEL_POW_H
