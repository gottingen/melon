//
// Created by liyinbin on 2020/2/28.
//

#ifndef ABEL_INCOMPLETE_GAMA_H
#define ABEL_INCOMPLETE_GAMA_H

#include <abel/math/option.h>
#include <abel/math/quadrature/gauss_legendre_50.h>
#include <abel/math/is_odd.h>
#include <abel/math/is_nan.h>
#include <abel/math/min.h>
#include <abel/math/max.h>
#include <abel/math/log_gamma.h>

namespace abel {

    namespace math_internal {

        template<typename T>
        ABEL_CONSTEXPR T
        incomplete_gamma_quad_inp_vals(const T lb, const T ub, const int counter) ABEL_NOEXCEPT {
            return (ub - lb) * gauss_legendre_50_points[counter] / T(2) + (ub + lb) / T(2);
        }

        template<typename T>
        ABEL_CONSTEXPR T
        incomplete_gamma_quad_weight_vals(const T lb, const T ub, const int counter) ABEL_NOEXCEPT {
            return (ub - lb) * gauss_legendre_50_weights[counter] / T(2);
        }

        template<typename T>
        ABEL_CONSTEXPR T incomplete_gamma_quad_fn(const T x, const T a, const T lg_term) ABEL_NOEXCEPT {
            return exp(-x + (a - T(1)) * log(x) - lg_term);
        }

        template<typename T>
        ABEL_CONSTEXPR T incomplete_gamma_quad_recur(const T lb, const T ub, const T a, const T lg_term,
                                                              const int counter) ABEL_NOEXCEPT {
            return (counter < 49 ? incomplete_gamma_quad_fn(incomplete_gamma_quad_inp_vals(lb, ub, counter), a, lg_term)
                                   * incomplete_gamma_quad_weight_vals(lb, ub, counter)
                                   + incomplete_gamma_quad_recur(lb, ub, a, lg_term, counter + 1) :
                    incomplete_gamma_quad_fn(incomplete_gamma_quad_inp_vals(lb, ub, counter), a, lg_term)
                    * incomplete_gamma_quad_weight_vals(lb, ub, counter));
        }

        template<typename T>
        ABEL_CONSTEXPR T incomplete_gamma_quad_lb(const T a, const T z) ABEL_NOEXCEPT {
            return (a > T(1000) ? max(T(0), min(z, a) - 11 * sqrt(a)) : // break integration into ranges
                    a > T(800) ? max(T(0), min(z, a) - 11 * sqrt(a)) :
                    a > T(500) ? max(T(0), min(z, a) - 10 * sqrt(a)) :
                    a > T(300) ? max(T(0), min(z, a) - 10 * sqrt(a)) :
                    a > T(100) ? max(T(0), min(z, a) - 9 * sqrt(a)) :
                    a > T(90) ? max(T(0), min(z, a) - 9 * sqrt(a)) :
                    a > T(70) ? max(T(0), min(z, a) - 8 * sqrt(a)) :
                    a > T(50) ? max(T(0), min(z, a) - 7 * sqrt(a)) :
                    a > T(40) ? max(T(0), min(z, a) - 6 * sqrt(a)) :
                    a > T(30) ? max(T(0), min(z, a) - 5 * sqrt(a)) :
                    // else
                    max(T(0), min(z, a) - 4 * sqrt(a)));
        }

        template<typename T>
        ABEL_CONSTEXPR T incomplete_gamma_quad_ub(const T a, const T z) ABEL_NOEXCEPT {
            return (a > T(1000) ? min(z, a + 10 * sqrt(a)) :
                    a > T(800) ? min(z, a + 10 * sqrt(a)) :
                    a > T(500) ? min(z, a + 9 * sqrt(a)) :
                    a > T(300) ? min(z, a + 9 * sqrt(a)) :
                    a > T(100) ? min(z, a + 8 * sqrt(a)) :
                    a > T(90) ? min(z, a + 8 * sqrt(a)) :
                    a > T(70) ? min(z, a + 7 * sqrt(a)) :
                    a > T(50) ? min(z, a + 6 * sqrt(a)) :
                    a > T(40) ? min(z, a + 5 * sqrt(a)) :
                    // else
                    min(z, a + 4 * sqrt(a)));
        }

        template<typename T>
        ABEL_CONSTEXPR T incomplete_gamma_quad(const T a, const T z) ABEL_NOEXCEPT {
            return incomplete_gamma_quad_recur(incomplete_gamma_quad_lb(a, z), incomplete_gamma_quad_ub(a, z), a,
                                               lgamma(a), 0);
        }

        // cf expansion
        // see: http://functions.wolfram.com/GammaBetaErf/Gamma2/10/0009/

        template<typename T>
        ABEL_CONSTEXPR T incomplete_gamma_cf_coef(const T a, const T z, const int depth) ABEL_NOEXCEPT {
            return (is_odd(depth) ? -(a - 1 + T(depth + 1) / T(2)) * z : T(depth) / T(2) * z);
        }

        template<typename T>
        ABEL_CONSTEXPR T incomplete_gamma_cf_recur(const T a, const T z, const int depth) ABEL_NOEXCEPT {
            return (depth < ABEL_INCML_GAMMA_MAX_ITER ? (a + depth - 1) + incomplete_gamma_cf_coef(a, z, depth) /
                                                                          incomplete_gamma_cf_recur(a, z, depth + 1) :
                    (a + depth - 1));
        }

        template<typename T>
        ABEL_CONSTEXPR T incomplete_gamma_cf(const T a, const T z) ABEL_NOEXCEPT {
            return (exp(a * log(z) - z) / tgamma(a) / incomplete_gamma_cf_recur(a, z, 1));
        }

//

        template<typename T>
        ABEL_CONSTEXPR T incomplete_gamma_check(const T a, const T z) ABEL_NOEXCEPT {
            return (any_nan(a, z) ? std::numeric_limits<T>::quiet_NaN() : a < T(0) ?
                                                                          std::numeric_limits<T>::quiet_NaN() :
                                                                          std::numeric_limits<T>::epsilon() > z ?
                                                                          T(0) :
                                                                          std::numeric_limits<T>::epsilon() > a ?
                                                                          T(1) :
                                                                          a < T(10) ?
                                                                          incomplete_gamma_cf(a, z) :
                                                                          incomplete_gamma_quad(a, z));
        }

        template<typename T1, typename T2, typename TC = common_return_t<T1, T2>>
        ABEL_CONSTEXPR TC incomplete_gamma_type_check(const T1 a, const T2 p) ABEL_NOEXCEPT {
            return incomplete_gamma_check(static_cast<TC>(a),
                                          static_cast<TC>(p));
        }

    } //namespace math_internal


    template<typename T1, typename T2>
    ABEL_CONSTEXPR common_return_t<T1, T2> incomplete_gamma(const T1 a, const T2 x) ABEL_NOEXCEPT {
        return math_internal::incomplete_gamma_type_check(a, x);
    }

} //namespace abel

#endif //ABEL_INCOMPLETE_GAMA_H
