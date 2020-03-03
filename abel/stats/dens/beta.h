//
// Created by liyinbin on 2020/3/3.
//

#ifndef ABEL_STATS_DENS_STATS_BETA_H_
#define ABEL_STATS_DENS_STATS_BETA_H_


#include <abel/base/profile.h>
#include <abel/stats/internal/beta_check.h>
#include <abel/math/all.h>

namespace abel {
    namespace stats_internal {

        template<typename T>
        ABEL_CONSTEXPR bool beta_sanity_check(const T inp_val, const T a_par, const T b_par)
        ABEL_NOEXCEPT {
            return (!abel::math_internal::is_nan(inp_val)) && beta_sanity_check(a_par, b_par);
        }


        template<typename T>
        ABEL_CONSTEXPR T beta_log_compute(const T x, const T a_par, const T b_par) ABEL_NOEXCEPT {
            return (-(abel::lgamma(a_par) + abel::lgamma(b_par) - abel::lgamma(a_par + b_par))
                    + (a_par - T(1)) * abel::log(x) + (b_par - T(1)) * abel::log(T(1) - x));
        }

        template<typename T>
        ABEL_CONSTEXPR T beta_limit_vals(const T x, const T a_par, const T b_par) ABEL_NOEXCEPT {
            return (a_par == T(0) && b_par == T(0) ?
                    x == T(0) || x == T(1) ? std::numeric_limits<T>::infinity() : T(0) :
                    a_par == T(0) || (abel::math_internal::is_posinf(b_par) && !abel::math_internal::is_posinf(a_par)) ?
                    x == T(0) ? std::numeric_limits<T>::infinity() : T(0) :
                    b_par == T(0) || (abel::math_internal::is_posinf(a_par) && !abel::math_internal::is_posinf(b_par)) ?
                    x == T(1) ? std::numeric_limits<T>::infinity() : T(0) :
                    x == T(0.5) ? std::numeric_limits<T>::infinity() : T(0));
        }

        template<typename T>
        ABEL_CONSTEXPR T beta_vals_check(const T x, const T a_par, const T b_par, const bool log_form) ABEL_NOEXCEPT {
            return (!beta_sanity_check(x, a_par, b_par) ?
                    std::numeric_limits<T>::quiet_NaN() :
                    x < T(0) || x > T(1) ?
                    abel::log_zero_if<T>(log_form) :
                    (a_par == T(0) || b_par == T(0) || abel::math_internal::any_posinf(a_par, b_par)) ?
                    abel::log_if(beta_limit_vals(x, a_par, b_par), log_form) :
                    x == T(0) ?
                    a_par < T(1) ?
                    std::numeric_limits<T>::infinity() :
                    a_par > T(1) ?
                    log_zero_if<T>(log_form) :
                    log_if(b_par, log_form) :
                    x == T(1) ?
                    b_par < T(1) ?
                    std::numeric_limits<T>::infinity() :
                    b_par > T(1) ?
                    log_zero_if<T>(log_form) :
                    log_if(a_par, log_form) :
                    exp_if(beta_log_compute(x, a_par, b_par), !log_form));
        }

        template<typename T1, typename T2, typename T3, typename TC = common_return_t<T1, T2, T3>>
        ABEL_CONSTEXPR TC
        beta_type_check(const T1 x, const T2 a_par, const T3 b_par, const bool log_form) ABEL_NOEXCEPT {
            return beta_vals_check(static_cast<TC>(x), static_cast<TC>(a_par),
                                   static_cast<TC>(b_par), log_form);
        }

    } //namespace stats_internal

    template<typename T1, typename T2, typename T3>
    ABEL_CONSTEXPR
    common_return_t<T1, T2, T3>
    pdf_beta(const T1 x, const T2 a_par, const T3 b_par, const bool log_form)
    ABEL_NOEXCEPT {
        return stats_internal::beta_type_check(x, a_par, b_par, log_form);
    }

} //namespace abel

#endif //ABEL_STATS_DENS_STATS_BETA_H_
