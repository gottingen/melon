//
// Created by liyinbin on 2020/2/28.
//

#ifndef ABEL_MATH_FACTORIAL_H_
#define ABEL_MATH_FACTORIAL_H_

#include <abel/math/option.h>

namespace abel {

    namespace math_internal {

        template<typename T>
        ABEL_CONSTEXPR T factorial_table(const T x) ABEL_NOEXCEPT {
            return (x == T(2) ? T(2) : x == T(3) ?
                                       T(6) : x == T(4) ?
                                              T(24) : x == T(5) ?
                                                      T(120) : x == T(6) ?
                                                               T(720) : x == T(7) ?
                                                                        T(5040) : x == T(8) ?
                                                                                  T(40320) : x == T(9) ?
                                                                                             T(362880) : x == T(10) ?
                                                                                                         T(3628800) :
                                                                                                         x == T(11) ?
                                                                                                         T(39916800) :
                                                                                                         x == T(12) ?
                                                                                                         T(479001600) :
                                                                                                         x == T(13) ?
                                                                                                         T(6227020800) :
                                                                                                         x == T(14) ?
                                                                                                         T(87178291200)
                                                                                                                    :
                                                                                                         x == T(15) ?
                                                                                                         T(1307674368000)
                                                                                                                    : T(
                                                                                                                 20922789888000));
        }

        template<typename T, typename std::enable_if<std::is_integral<T>::value>::type * = nullptr>
        ABEL_CONSTEXPR T factorial_recur(const T x) ABEL_NOEXCEPT {
            return (x == T(0) ? T(1) : x == T(1) ?
                                       x : x < T(17) ?
                                           factorial_table(x) : x * factorial_recur(x - 1));
        }

        template<typename T, typename std::enable_if<!std::is_integral<T>::value>::type * = nullptr>
        ABEL_CONSTEXPR T factorial_recur(const T x) ABEL_NOEXCEPT {
            return tgamma(x + 1);
        }

    } //namespace math_internal

    template<typename T>
    ABEL_DEPRECATED_MESSAGE("use std version instead")
    ABEL_CONSTEXPR T factorial(const T x) ABEL_NOEXCEPT {
        return math_internal::factorial_recur(x);
    }

}
#endif //ABEL_MATH_FACTORIAL_H_
