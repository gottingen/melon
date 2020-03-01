//
// Created by liyinbin on 2020/2/28.
//

#ifndef ABEL_INCOMPLETE_GAMA_INV_H
#define ABEL_INCOMPLETE_GAMA_INV_H

#include <abel/math/option.h>
#include <abel/math/is_nan.h>
#include <abel/math/max.h>
#include <abel/math/min.h>
#include <abel/math//incomplete_gamma.h>
#include <abel/math/log_gamma.h>

namespace abel {

    namespace math_internal {

        template<typename T>
        ABEL_CONSTEXPR T
        incomplete_gamma_inv_decision(const T value, const T a, const T p, const T direc, const T lg_val,
                                      const int iter_count) ABEL_NOEXCEPT;

//
// initial value for Halley

        template<typename T>
        ABEL_CONSTEXPR T incomplete_gamma_inv_t_val_1(const T p) ABEL_NOEXCEPT {   // a > 1.0
            return (p > T(0.5) ? sqrt(-2 * log(T(1) - p)) : sqrt(-2 * log(p)));
        }

        template<typename T>
        ABEL_CONSTEXPR T incomplete_gamma_inv_t_val_2(const T a) ABEL_NOEXCEPT {   // a <= 1.0
            return (T(1) - T(0.253) * a - T(0.12) * a * a);
        }

//

        template<typename T>
        ABEL_CONSTEXPR T
        incomplete_gamma_inv_initial_val_1_int_begin(const T t_val) ABEL_NOEXCEPT {
            return (t_val - (T(2.515517L) + T(0.802853L) * t_val + T(0.010328L) * t_val * t_val)
                            / (T(1) + T(1.432788L) * t_val + T(0.189269L) * t_val * t_val +
                               T(0.001308L) * t_val * t_val * t_val));
        }

        template<typename T>
        ABEL_CONSTEXPR T incomplete_gamma_inv_initial_val_1_int_end(const T value_inp,
                                                                             const T a) ABEL_NOEXCEPT {
            return max(T(1E-04), a * pow(T(1) - T(1) / (9 * a) - value_inp / (3 * sqrt(a)), 3));
        }

        template<typename T>
        ABEL_CONSTEXPR T
        incomplete_gamma_inv_initial_val_1(const T a, const T t_val, const T sgn_term) ABEL_NOEXCEPT {   // a > 1.0
            return incomplete_gamma_inv_initial_val_1_int_end(
                    sgn_term * incomplete_gamma_inv_initial_val_1_int_begin(t_val), a);
        }

        template<typename T>
        ABEL_CONSTEXPR T
        incomplete_gamma_inv_initial_val_2(const T a, const T p, const T t_val) ABEL_NOEXCEPT {   // a <= 1.0
            return (p < t_val ? \
             // if
                    pow(p / t_val, T(1) / a) :
                    // else
                    T(1) - log(T(1) - (p - t_val) / (T(1) - t_val)));
        }

// initial value

        template<typename T>
        ABEL_CONSTEXPR T incomplete_gamma_inv_initial_val(const T a, const T p) ABEL_NOEXCEPT {
            return (a > T(1) ?
                    incomplete_gamma_inv_initial_val_1(a, incomplete_gamma_inv_t_val_1(p), p > T(0.5) ? T(-1) : T(1)) :
                    incomplete_gamma_inv_initial_val_2(a, p, incomplete_gamma_inv_t_val_2(a)));
        }

        template<typename T>
        ABEL_CONSTEXPR T incomplete_gamma_inv_err_val(const T value, const T a, const T p) ABEL_NOEXCEPT {
            return (incomplete_gamma(a, value) - p);
        }

        template<typename T>
        ABEL_CONSTEXPR T incomplete_gamma_inv_deriv_1(const T value, const T a,
                                                               const T lg_val) ABEL_NOEXCEPT {
            return (exp(-value + (a - T(1)) * log(value) - lg_val));
        }

        template<typename T>
        ABEL_CONSTEXPR T
        incomplete_gamma_inv_deriv_2(const T value, const T a, const T deriv_1) ABEL_NOEXCEPT {
            return (deriv_1 * ((a - T(1)) / value - T(1)));
        }

        template<typename T>
        ABEL_CONSTEXPR T
        incomplete_gamma_inv_ratio_val_1(const T value, const T a, const T p, const T deriv_1) ABEL_NOEXCEPT {
            return (incomplete_gamma_inv_err_val(value, a, p) / deriv_1);
        }

        template<typename T>
        ABEL_CONSTEXPR T
        incomplete_gamma_inv_ratio_val_2(const T value, const T a, const T deriv_1) ABEL_NOEXCEPT {
            return (incomplete_gamma_inv_deriv_2(value, a, deriv_1) / deriv_1);
        }

        template<typename T>
        ABEL_CONSTEXPR T incomplete_gamma_inv_halley(const T ratio_val_1, const T ratio_val_2) ABEL_NOEXCEPT {
            return (ratio_val_1 / max(T(0.8), min(T(1.2), T(1) - T(0.5) * ratio_val_1 * ratio_val_2)));
        }

        template<typename T>
        ABEL_CONSTEXPR T
        incomplete_gamma_inv_recur(const T value, const T a, const T p, const T deriv_1, const T lg_val,
                                   const int iter_count) ABEL_NOEXCEPT {
            return incomplete_gamma_inv_decision(value, a, p,
                                                 incomplete_gamma_inv_halley(
                                                         incomplete_gamma_inv_ratio_val_1(value, a, p, deriv_1),
                                                         incomplete_gamma_inv_ratio_val_2(value, a, deriv_1)),
                                                 lg_val, iter_count);
        }

        template<typename T>
        ABEL_CONSTEXPR T
        incomplete_gamma_inv_decision(const T value, const T a, const T p, const T direc, const T lg_val,
                                      const int iter_count) ABEL_NOEXCEPT {
            // return( abs(direc) > ABEL_INCML_GAMMA_INV_TOL ? incomplete_gamma_inv_recur(value - direc, a, p, incomplete_gamma_inv_deriv_1(value,a,lg_val), lg_val) : value - direc );
            return (iter_count <= ABEL_INCML_GAMMA_INV_MAX_ITER ?
                    incomplete_gamma_inv_recur(value - direc, a, p,
                                               incomplete_gamma_inv_deriv_1(value, a, lg_val),
                                               lg_val, iter_count + 1) :
                    value - direc);
        }

        template<typename T>
        ABEL_CONSTEXPR T
        incomplete_gamma_inv_begin(const T initial_val, const T a, const T p, const T lg_val) ABEL_NOEXCEPT {
            return incomplete_gamma_inv_recur(initial_val, a, p,
                                              incomplete_gamma_inv_deriv_1(initial_val, a, lg_val), lg_val, 1);
        }

        template<typename T>
        ABEL_CONSTEXPR T incomplete_gamma_inv_check(const T a, const T p) ABEL_NOEXCEPT {
            return (any_nan(a, p) ?
                    std::numeric_limits<T>::quiet_NaN() :
                    std::numeric_limits<T>::epsilon() > p ?
                    T(0) :
                    p > T(1) ?
                    std::numeric_limits<T>::quiet_NaN() :
                    std::numeric_limits<T>::epsilon() > abs(T(1) - p) ?
                    std::numeric_limits<T>::infinity() :
                    std::numeric_limits<T>::epsilon() > a ?
                    T(0) :
                    incomplete_gamma_inv_begin(incomplete_gamma_inv_initial_val(a, p), a, p, lgamma(a)));
        }

        template<typename T1, typename T2, typename TC = common_return_t<T1, T2>>
        ABEL_CONSTEXPR TC incomplete_gamma_inv_type_check(const T1 a, const T2 p) ABEL_NOEXCEPT {
            return incomplete_gamma_inv_check(static_cast<TC>(a),
                                              static_cast<TC>(p));
        }

    } //namespace math_internal


    template<typename T1, typename T2>
    ABEL_CONSTEXPR common_return_t<T1, T2> incomplete_gamma_inv(const T1 a, const T2 p) ABEL_NOEXCEPT {
        return math_internal::incomplete_gamma_inv_type_check(a, p);
    }

} //namespace abel

#endif //ABEL_INCOMPLETE_GAMA_INV_H
