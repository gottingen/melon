//
// Created by liyinbin on 2020/3/3.
//

#ifndef ABEL_STATS_DENS_BINOMIAL_H_
#define ABEL_STATS_DENS_BINOMIAL_H_

#include <abel/stats/internal/stats_option.h>
#include <abel/stats/internal/binom_check.h>
#include <abel/stats/dens/bernoulli.h>

namespace abel {

    namespace stats_internal {

        template<typename T>
        ABEL_CONSTEXPR T
        binom_log_compute(const long long x, const long long n_trials_par, const T prob_par) ABEL_NOEXCEPT {
            return (x == (long long)(0) ? n_trials_par * abel::log(T(1.0) - prob_par) :
                    x == n_trials_par ? x * abel::log(prob_par) : abel::log_binomial_coef(n_trials_par, x) +
                                                                  x * abel::log(prob_par)
                                                                  + (n_trials_par - x) *
                                                                    abel::log(T(1) - prob_par));
        }

        template<typename T>
        ABEL_CONSTEXPR
        T
        binom_limit_vals(const long long x) ABEL_NOEXCEPT {
            return (x == (long long)(0) ?
                    T(1) :
                    T(0));
        }

        template<typename T>
        ABEL_CONSTEXPR T binom_vals_check(const long long x, const long long n_trials_par, const T prob_par,
                                          const bool log_form) ABEL_NOEXCEPT {
            return (!binom_sanity_check(n_trials_par, prob_par) ?
                    std::numeric_limits<T>::quiet_NaN() :
                    abel::math_internal::is_nan(x) ?
                    std::numeric_limits<T>::quiet_NaN() :
                    x < (long long) (0) || x > n_trials_par ?
                    abel::log_zero_if<T>(log_form) :
                    n_trials_par == (long long) (0) ?
                    abel::log_if(binom_limit_vals<T>(x), log_form) :
                    n_trials_par == (long long) (1) ?
                    abel::pdf_bern(x, prob_par, log_form) :
                    abel::exp_if(binom_log_compute(x, n_trials_par, prob_par), !log_form));
        }

    } //namespace stats_internal


    template<typename T>
    ABEL_CONSTEXPR return_t<T>
    pdf_binom(const long long x, const long long n_trials_par, const T prob_par, const bool log_form) ABEL_NOEXCEPT {
        return stats_internal::binom_vals_check(x, n_trials_par, static_cast<return_t<T>>(prob_par), log_form);
    }

} //namespace abel
#endif //ABEL_STATS_DENS_BINOMIAL_H_
