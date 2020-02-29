//
// Created by liyinbin on 2020/2/28.
//

#ifndef ABEL_LOG_PLUS_ONE_H
#define ABEL_LOG_PLUS_ONE_H

#include <abel/math/option.h>
#include <abel/math/is_nan.h>

namespace abel {

    namespace math_internal {

        // see:
        // http://functions.wolfram.com/ElementaryFunctions/Log/06/01/04/01/0003/


        template<typename T>
        ABEL_CONSTEXPR T log1p_compute(const T x) ABEL_NOEXCEPT {
            // return x * ( T(1) + x * ( -T(1)/T(2) +  x * ( T(1)/T(3) +  x * ( -T(1)/T(4) + x/T(5) ) ) ) ); // O(x^6)
            return x + x * (-x / T(2) + x * (x / T(3) + x * (-x / T(4) + x * x / T(5)))); // O(x^6)
        }

        template<typename T>
        ABEL_CONSTEXPR T log1p_check(const T x) ABEL_NOEXCEPT {
            return (is_nan(x) ? std::numeric_limits<T>::quiet_NaN() : abs(x) > T(1e-04) ? log(T(1) + x) : log1p_compute(
                    x));
        }

    }


    template<typename T>
    ABEL_CONSTEXPR return_t<T> log1p(const T x) ABEL_NOEXCEPT {
        return math_internal::log1p_check(static_cast<return_t<T>>(x));
    }

}
#endif //ABEL_LOG_PLUS_ONE_H
