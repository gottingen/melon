//
// Created by liyinbin on 2020/2/28.
//

#ifndef ABEL_INCOMPLETE_BETA_H
#define ABEL_INCOMPLETE_BETA_H

#include <abel/math/option.h>
#include <abel/math/is_odd.h>
#include <abel/math/is_nan.h>
#include <abel/math/log_beta.h>

namespace abel {

    namespace math_internal {

        template<typename T>
        ABEL_CONSTEXPR T
        incomplete_beta_cf(const T a, const T b, const T z, const T c_j, const T d_j, const T f_j,
                           const int depth) ABEL_NOEXCEPT;


        template<typename T>
        ABEL_CONSTEXPR T
        incomplete_beta_coef_even(const T a, const T b, const T z, const int k) ABEL_NOEXCEPT {
            return (-z * (a + k) * (a + b + k) / ((a + 2 * k) * (a + 2 * k + T(1))));
        }

        template<typename T>
        ABEL_CONSTEXPR T incomplete_beta_coef_odd(const T a, const T b, const T z, const int k) ABEL_NOEXCEPT {
            return (z * k * (b - k) / ((a + 2 * k - T(1)) * (a + 2 * k)));
        }

        template<typename T>
        ABEL_CONSTEXPR T incomplete_beta_coef(const T a, const T b, const T z, const int depth) ABEL_NOEXCEPT {
            return (!is_odd(depth) ? incomplete_beta_coef_even(a, b, z, depth / 2) :
                    incomplete_beta_coef_odd(a, b, z, (depth + 1) / 2));
        }


        template<typename T>
        ABEL_CONSTEXPR T
        incomplete_beta_c_update(const T a, const T b, const T z, const T c_j, const int depth) ABEL_NOEXCEPT {
            return (T(1) + incomplete_beta_coef(a, b, z, depth) / c_j);
        }

        template<typename T>
        ABEL_CONSTEXPR T
        incomplete_beta_d_update(const T a, const T b, const T z, const T d_j, const int depth) ABEL_NOEXCEPT {
            return (T(1) / (T(1) + incomplete_beta_coef(a, b, z, depth) * d_j));
        }

        template<typename T>
        ABEL_CONSTEXPR T
        incomplete_beta_decision(const T a, const T b, const T z, const T c_j, const T d_j, const T f_j,
                                 const int depth) ABEL_NOEXCEPT {
            return (abs(c_j * d_j - T(1)) < ABEL_INCML_BETA_TOL ? f_j * c_j * d_j :
                    depth < ABEL_INCML_BETA_MAX_ITER ?
                    incomplete_beta_cf(a, b, z, c_j, d_j, f_j * c_j * d_j, depth + 1) :
                    f_j * c_j * d_j);
        }

        template<typename T>
        ABEL_CONSTEXPR T
        incomplete_beta_cf(const T a, const T b, const T z, const T c_j, const T d_j, const T f_j,
                           const int depth) ABEL_NOEXCEPT {
            return incomplete_beta_decision(a, b, z,
                                            incomplete_beta_c_update(a, b, z, c_j, depth),
                                            incomplete_beta_d_update(a, b, z, d_j, depth),
                                            f_j, depth);
        }

        template<typename T>
        ABEL_CONSTEXPR T
        incomplete_beta_begin(const T a, const T b, const T z) ABEL_NOEXCEPT {
            return ((exp(a * log(z) + b * log(T(1) - z) - lbeta(a, b)) / a) *
                    incomplete_beta_cf(a, b, z, T(1),
                                       incomplete_beta_d_update(a, b, z, T(1), 0),
                                       incomplete_beta_d_update(a, b, z, T(1), 0), 1)
            );
        }

        template<typename T>
        ABEL_CONSTEXPR T incomplete_beta_check(const T a, const T b, const T z) ABEL_NOEXCEPT {
            return (any_nan(a, b, z) ?
                    std::numeric_limits<T>::quiet_NaN() :
                    std::numeric_limits<T>::epsilon() > z ?
                    T(0) :
                    (a + T(1)) / (a + b + T(2)) > z ?
                    incomplete_beta_begin(a, b, z) :
                    T(1) - incomplete_beta_begin(b, a, T(1) - z));
        }

        template<typename T1, typename T2, typename T3, typename TC = common_return_t<T1, T2, T3>>
        ABEL_CONSTEXPR TC incomplete_beta_type_check(const T1 a, const T2 b, const T3 p) ABEL_NOEXCEPT {
            return incomplete_beta_check(static_cast<TC>(a),
                                         static_cast<TC>(b),
                                         static_cast<TC>(p));
        }

    } //namespace math_internal


    template<typename T1, typename T2, typename T3>
    ABEL_CONSTEXPR common_return_t<T1, T2, T3>
    incomplete_beta(const T1 a, const T2 b, const T3 z) ABEL_NOEXCEPT {
        return math_internal::incomplete_beta_type_check(a, b, z);
    }

}
#endif //ABEL_INCOMPLETE_BETA_H
