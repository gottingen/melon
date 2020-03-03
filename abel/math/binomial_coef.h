//
// Created by liyinbin on 2020/2/28.
//

#ifndef ABEL_MATH_BINOMIAL_COEF_H_
#define ABEL_MATH_BINOMIAL_COEF_H_

#include <abel/math/option.h>

namespace abel {

    namespace math_internal {

        template<typename T>
        ABEL_CONSTEXPR T binomial_coef_recur(const T n, const T k) ABEL_NOEXCEPT {
            return ((k == T(0) || n == k) ? T(1) : n == T(0) ? T(0) : binomial_coef_recur(n - 1, k - 1) +
                                                                      binomial_coef_recur(n - 1, k)
            );
        }

        template<typename T, typename std::enable_if<std::is_integral<T>::value>::type * = nullptr>
        ABEL_CONSTEXPR T binomial_coef_check(const T n, const T k) ABEL_NOEXCEPT {
            return binomial_coef_recur(n, k);
        }

        template<typename T, typename std::enable_if<!std::is_integral<T>::value>::type * = nullptr>
        ABEL_CONSTEXPR T binomial_coef_check(const T n, const T k) ABEL_NOEXCEPT {
            return ( // NaN check; removed due to MSVC problems; template not being ignored in <int> cases
                    // (is_nan(n) || is_nan(k)) ? std::numeric_limits<T>::quiet_NaN() :
                    //
                    static_cast<T>(binomial_coef_recur(static_cast<unsigned long long>(n), static_cast<unsigned long long>(k))));
        }

        template<typename T1, typename T2, typename TC = common_t<T1, T2>>
        ABEL_CONSTEXPR TC binomial_coef_type_check(const T1 n, const T2 k) ABEL_NOEXCEPT {
            return binomial_coef_check(static_cast<TC>(n), static_cast<TC>(k));
        }

    } //namespace math_internal


    template<typename T1, typename T2>
    ABEL_CONSTEXPR common_t<T1, T2> binomial_coef(const T1 n, const T2 k) ABEL_NOEXCEPT {
        return math_internal::binomial_coef_type_check(n, k);
    }

}
#endif //ABEL_MATH_BINOMIAL_COEF_H_
