//
// Created by liyinbin on 2020/2/28.
//

#ifndef ABEL_MATH_INCOMPLETE_BETA_INV_H_
#define ABEL_MATH_INCOMPLETE_BETA_INV_H_

#include <abel/math/option.h>
#include <abel/math/is_nan.h>
#include <abel/math/log_beta.h>
#include <abel/math/incomplete_beta.h>
#include <abel/math/max.h>
#include <abel/math//min.h>

namespace abel {

    namespace math_internal {

        template<typename T>
        ABEL_CONSTEXPR T
        incomplete_beta_inv_decision(const T value, const T alpha_par, const T beta_par, const T p,
                                     const T direc, const T lb_val, const int iter_count) ABEL_NOEXCEPT;

//
// initial value for Halley

//
// a,b > 1 case

        template<typename T>
        ABEL_CONSTEXPR T incomplete_beta_inv_initial_val_1_tval(const T p) ABEL_NOEXCEPT {
            return (p > T(0.5) ? sqrt(-T(2) * log(T(1) - p)) : sqrt(-T(2) * log(p)));
        }

        template<typename T>
        ABEL_CONSTEXPR T incomplete_beta_inv_initial_val_1_int_begin(const T t_val) ABEL_NOEXCEPT {
            return (t_val - (T(2.515517) + T(0.802853) * t_val + T(0.010328) * t_val * t_val) \
 / (T(1) + T(1.432788) * t_val + T(0.189269) * t_val * t_val + T(0.001308) * t_val * t_val * t_val));
        }

        template<typename T>
        ABEL_CONSTEXPR T
        incomplete_beta_inv_initial_val_1_int_ab1(const T alpha_par, const T beta_par) ABEL_NOEXCEPT {
            return (T(1) / (2 * alpha_par - T(1)) + T(1) / (2 * beta_par - T(1)));
        }

        template<typename T>
        ABEL_CONSTEXPR T
        incomplete_beta_inv_initial_val_1_int_ab2(const T alpha_par, const T beta_par) ABEL_NOEXCEPT {
            return (T(1) / (2 * beta_par - T(1)) - T(1) / (2 * alpha_par - T(1)));
        }

        template<typename T>
        ABEL_CONSTEXPR T incomplete_beta_inv_initial_val_1_int_h(const T ab_term_1) ABEL_NOEXCEPT {
            return (T(2) / ab_term_1);
        }

        template<typename T>
        ABEL_CONSTEXPR T
        incomplete_beta_inv_initial_val_1_int_w(const T value, const T ab_term_2, const T h_term) ABEL_NOEXCEPT {
            // return( value * sqrt(h_term + lambda)/h_term - ab_term_2*(lambda + 5.0/6.0 -2.0/(3.0*h_term)) );
            return (value * sqrt(h_term + (value * value - T(3)) / T(6)) / h_term - ab_term_2 *
                                                                                    ((value * value - T(3)) / T(6) +
                                                                                     T(5) / T(6) -
                                                                                     T(2) / (T(3) * h_term)));
        }

        template<typename T>
        ABEL_CONSTEXPR T
        incomplete_beta_inv_initial_val_1_int_end(const T alpha_par, const T beta_par, const T w_term) ABEL_NOEXCEPT {
            return (alpha_par / (alpha_par + beta_par * exp(2 * w_term)));
        }

        template<typename T>
        ABEL_CONSTEXPR T incomplete_beta_inv_initial_val_1(const T alpha_par, const T beta_par, const T t_val,
                                                           const T sgn_term) ABEL_NOEXCEPT {   // a > 1.0
            return incomplete_beta_inv_initial_val_1_int_end(alpha_par, beta_par,
                                                             incomplete_beta_inv_initial_val_1_int_w(
                                                                     sgn_term *
                                                                     incomplete_beta_inv_initial_val_1_int_begin(t_val),
                                                                     incomplete_beta_inv_initial_val_1_int_ab2(
                                                                             alpha_par, beta_par),
                                                                     incomplete_beta_inv_initial_val_1_int_h(
                                                                             incomplete_beta_inv_initial_val_1_int_ab1(
                                                                                     alpha_par, beta_par)
                                                                     )
                                                             )
            );
        }

//
// a,b else

        template<typename T>
        ABEL_CONSTEXPR T
        incomplete_beta_inv_initial_val_2_s1(const T alpha_par, const T beta_par) ABEL_NOEXCEPT {
            return (pow(alpha_par / (alpha_par + beta_par), alpha_par) / alpha_par);
        }

        template<typename T>
        ABEL_CONSTEXPR T
        incomplete_beta_inv_initial_val_2_s2(const T alpha_par, const T beta_par) ABEL_NOEXCEPT {
            return (pow(beta_par / (alpha_par + beta_par), beta_par) / beta_par);
        }

        template<typename T>
        ABEL_CONSTEXPR T
        incomplete_beta_inv_initial_val_2(const T alpha_par, const T beta_par, const T p, const T s_1,
                                          const T s_2) ABEL_NOEXCEPT {
            return (p <= s_1 / (s_1 + s_2) ? pow(p * (s_1 + s_2) * alpha_par, T(1) / alpha_par) :
                    T(1) - pow(p * (s_1 + s_2) * beta_par, T(1) / beta_par));
        }

// initial value

        template<typename T>
        ABEL_CONSTEXPR T
        incomplete_beta_inv_initial_val(const T alpha_par, const T beta_par, const T p) ABEL_NOEXCEPT {
            return ((alpha_par > T(1) && beta_par > T(1)) ? incomplete_beta_inv_initial_val_1(alpha_par, beta_par,
                                                                                              incomplete_beta_inv_initial_val_1_tval(
                                                                                                      p),
                                                                                              p < T(0.5) ? T(1) : T(-1))
                                                          :
                    p > T(0.5) ?
                    T(1) - incomplete_beta_inv_initial_val_2(beta_par, alpha_par, T(1) - p,
                                                             incomplete_beta_inv_initial_val_2_s1(beta_par, alpha_par),
                                                             incomplete_beta_inv_initial_val_2_s2(beta_par, alpha_par))
                               :
                    incomplete_beta_inv_initial_val_2(alpha_par, beta_par, p,
                                                      incomplete_beta_inv_initial_val_2_s1(alpha_par, beta_par),
                                                      incomplete_beta_inv_initial_val_2_s2(alpha_par, beta_par))
            );
        }

//
// Halley recursion

        template<typename T>
        ABEL_CONSTEXPR T
        incomplete_beta_inv_err_val(const T value, const T alpha_par, const T beta_par, const T p) ABEL_NOEXCEPT {
            return (incomplete_beta(alpha_par, beta_par, value) - p);
        }

        template<typename T>
        ABEL_CONSTEXPR T
        incomplete_beta_inv_deriv_1(const T value, const T alpha_par, const T beta_par, const T lb_val) ABEL_NOEXCEPT {
            return (std::numeric_limits<T>::epsilon() > abs(value) ? T(0) :
                    std::numeric_limits<T>::epsilon() > abs(T(1) - value) ? T(0) :
                    exp((alpha_par - T(1)) * log(value) + (beta_par - T(1)) * log(T(1) - value) - lb_val));
        }

        template<typename T>
        ABEL_CONSTEXPR T
        incomplete_beta_inv_deriv_2(const T value, const T alpha_par, const T beta_par, const T deriv_1) ABEL_NOEXCEPT {
            return (deriv_1 * ((alpha_par - T(1)) / value - (beta_par - T(1)) / (T(1) - value)));
        }

        template<typename T>
        ABEL_CONSTEXPR T
        incomplete_beta_inv_ratio_val_1(const T value, const T alpha_par, const T beta_par, const T p,
                                        const T deriv_1) ABEL_NOEXCEPT {
            return (incomplete_beta_inv_err_val(value, alpha_par, beta_par, p) / deriv_1);
        }

        template<typename T>
        ABEL_CONSTEXPR T incomplete_beta_inv_ratio_val_2(const T value, const T alpha_par, const T beta_par,
                                                         const T deriv_1) ABEL_NOEXCEPT {
            return (incomplete_beta_inv_deriv_2(value, alpha_par, beta_par, deriv_1) / deriv_1);
        }

        template<typename T>
        ABEL_CONSTEXPR T incomplete_beta_inv_halley(const T ratio_val_1, const T ratio_val_2) ABEL_NOEXCEPT {
            return (ratio_val_1 / max(T(0.8), min(T(1.2), T(1) - T(0.5) * ratio_val_1 * ratio_val_2)));
        }

        template<typename T>
        ABEL_CONSTEXPR T
        incomplete_beta_inv_recur(const T value, const T alpha_par, const T beta_par, const T p, const T deriv_1,
                                  const T lb_val, const int iter_count) ABEL_NOEXCEPT {
            return (
                    std::numeric_limits<T>::epsilon() > abs(deriv_1) ?
                    incomplete_beta_inv_decision(value, alpha_par, beta_par, p, T(0), lb_val,
                                                 ABEL_INCML_BETA_INV_MAX_ITER + 1) :
                    incomplete_beta_inv_decision(value, alpha_par, beta_par, p,
                                                 incomplete_beta_inv_halley(
                                                         incomplete_beta_inv_ratio_val_1(value, alpha_par, beta_par, p,
                                                                                         deriv_1),
                                                         incomplete_beta_inv_ratio_val_2(value, alpha_par, beta_par,
                                                                                         deriv_1)
                                                 ), lb_val, iter_count));
        }

        template<typename T>
        ABEL_CONSTEXPR T
        incomplete_beta_inv_decision(const T value, const T alpha_par, const T beta_par, const T p, const T direc,
                                     const T lb_val, const int iter_count) ABEL_NOEXCEPT {
            return (iter_count <= ABEL_INCML_BETA_INV_MAX_ITER ?
                    incomplete_beta_inv_recur(value - direc, alpha_par, beta_par, p,
                                              incomplete_beta_inv_deriv_1(value, alpha_par, beta_par, lb_val),
                                              lb_val, iter_count + 1) :
                    value - direc);
        }

        template<typename T>
        ABEL_CONSTEXPR T
        incomplete_beta_inv_begin(const T initial_val, const T alpha_par, const T beta_par, const T p,
                                  const T lb_val) ABEL_NOEXCEPT {
            return incomplete_beta_inv_recur(initial_val, alpha_par, beta_par, p,
                                             incomplete_beta_inv_deriv_1(initial_val, alpha_par, beta_par, lb_val),
                                             lb_val, 1);
        }

        template<typename T>
        ABEL_CONSTEXPR T
        incomplete_beta_inv_check(const T alpha_par, const T beta_par, const T p) ABEL_NOEXCEPT {
            return (any_nan(alpha_par, beta_par, p) ?
                    std::numeric_limits<T>::quiet_NaN() :
                    std::numeric_limits<T>::epsilon() > p ?
                    T(0) :
                    std::numeric_limits<T>::epsilon() > abs(T(1) - p) ?
                    T(1) :
                    incomplete_beta_inv_begin(incomplete_beta_inv_initial_val(alpha_par, beta_par, p),
                                              alpha_par, beta_par, p, lbeta(alpha_par, beta_par)));
        }

        template<typename T1, typename T2, typename T3, typename TC = common_t<T1, T2, T3>>
        ABEL_CONSTEXPR TC incomplete_beta_inv_type_check(const T1 a, const T2 b, const T3 p) ABEL_NOEXCEPT {
            return incomplete_beta_inv_check(static_cast<TC>(a),
                                             static_cast<TC>(b),
                                             static_cast<TC>(p));
        }

    } //namespace math_internal


    template<typename T1, typename T2, typename T3>
    ABEL_CONSTEXPR
    ABEL_DEPRECATED_MESSAGE("use std version instead")
    common_t<T1, T2, T3>
    incomplete_beta_inv(const T1 a, const T2 b, const T3 p)
    ABEL_NOEXCEPT {
        return math_internal::incomplete_beta_inv_type_check(a, b, p);
    }
} //namespace abel

#endif //ABEL_MATH_INCOMPLETE_BETA_INV_H_
