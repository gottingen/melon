
#ifndef ABEL_MATH_TAN_H_
#define ABEL_MATH_TAN_H_

#include <abel/math/option.h>
#include <abel/math/abs.h>
#include <abel/math/pow_integer.h>
#include <abel/math//floor.h>

namespace abel {
    namespace math_internal {
        template<typename T>
        ABEL_CONSTEXPR T tan_series_exp_long(const T z) ABEL_NOEXCEPT {
            return (-1 / z + (z / 3 + (pow_integral(z,3) / 45 + (2 *pow_integral(z,5) / 945 + pow_integral(z,7) / 4725))));
        }

        template<typename T> 
        ABEL_CONSTEXPR T tan_series_exp(const T x) ABEL_NOEXCEPT {
            return (std::numeric_limits<T>::epsilon() > abel::abs(x - T(ABEL_HALF_PI)) ?
                    T(1.633124e+16) : tan_series_exp_long(x - T(ABEL_HALF_PI))
            );
        }

        template<typename T>
        ABEL_CONSTEXPR T tan_cf_recur(const T xx, const int depth, const int max_depth) ABEL_NOEXCEPT {
            return (depth < max_depth ?
                T(2 * depth - 1) - xx / tan_cf_recur(xx, depth + 1, max_depth) :
                    T(2 * depth - 1)
            );
        }

        template<typename T>
        ABEL_CONSTEXPR T tan_cf_main(const T x) ABEL_NOEXCEPT {
            return ((x > T(1.55) && x < T(1.60)) ? tan_series_exp(x) : x > T(1.4) ? x / tan_cf_recur(x * x, 1, 45) :
                    x > T(1) ? x / tan_cf_recur(x * x, 1, 35) :
                    x / tan_cf_recur(x * x, 1, 25));
        }

        template<typename T>
        ABEL_CONSTEXPR T tan_begin(const T x, const int count = 0) ABEL_NOEXCEPT {   // tan(x) = tan(x + pi)
            return (x > T(ABEL_PI) ?
                      count > 1 ?
                      std::numeric_limits<T>::quiet_NaN() : // protect against undefined behavior
                      tan_begin(x - T(ABEL_PI) * math_internal::floor_check(x / T(ABEL_PI)), count + 1) :
                    tan_cf_main(x)
            );
        }

        template<typename T>
        ABEL_CONSTEXPR T tan_check(const T x) ABEL_NOEXCEPT {
            return ( // NaN check
                    is_nan(x) ?
                    std::numeric_limits<T>::quiet_NaN() :
                    std::numeric_limits<T>::epsilon() > abel::abs(x) ?
                    T(0) : x < T(0) ? -tan_begin(-x) :
                    tan_begin(x)
            );
        }
    } //namespace math_internal

    template<typename T>
    ABEL_DEPRECATED_MESSAGE("use std version instead")
    ABEL_CONSTEXPR return_t<T> tan(const T x) ABEL_NOEXCEPT {
        return math_internal::tan_check( static_cast<return_t<T>>(x) );
    }

} //namespace abel
#endif //ABEL_MATH_TAN_H_
