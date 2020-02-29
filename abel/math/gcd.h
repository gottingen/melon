//
// Created by liyinbin on 2020/2/28.
//

#ifndef ABEL_GCD_H
#define ABEL_GCD_H

#include <abel/math/option.h>

namespace abel {

    namespace math_internal {

        template<typename T>
        ABEL_CONSTEXPR T gcd_recur(const T a, const T b) ABEL_NOEXCEPT {
            return (b == T(0) ? a : gcd_recur(b, a % b));
        }

        template<typename T, typename std::enable_if<std::is_integral<T>::value>::type * = nullptr>
        ABEL_CONSTEXPR T gcd_int_check(const T a, const T b) ABEL_NOEXCEPT {
            return gcd_recur(a, b);
        }

        template<typename T, typename std::enable_if<!std::is_integral<T>::value>::type * = nullptr>
        ABEL_CONSTEXPR T gcd_int_check(const T a, const T b) ABEL_NOEXCEPT {
            return gcd_recur(static_cast<unsigned long long>(a), static_cast<unsigned long long>(b));
        }

        template<typename T1, typename T2, typename TC = common_t<T1, T2>>
        ABEL_CONSTEXPR TC gcd_type_check(const T1 a, const T2 b) ABEL_NOEXCEPT {
            return gcd_int_check(static_cast<TC>(abs(a)), static_cast<TC>(abs(b)));
        }

    } //namespace math_internal


    template<typename T1, typename T2>
    ABEL_CONSTEXPR
    common_t<T1, T2>
    gcd(const T1 a, const T2 b)
    ABEL_NOEXCEPT {
        return math_internal::gcd_type_check(a, b);
    }
}
#endif //ABEL_GCD_H
