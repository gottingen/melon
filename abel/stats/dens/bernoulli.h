//
// Created by liyinbin on 2020/3/3.
//

#ifndef ABEL_STATS_DENS_BERNOULLI_H_
#define ABEL_STATS_DENS_BERNOULLI_H_

#include <abel/stats/internal/bern_check.h>
#include <abel/stats/internal/stats_option.h>
#include <cstdint>
#include <cstddef>


namespace abel {

    namespace stats_internal {


        template<typename T>
        ABEL_CONSTEXPR T bern_compute(const long long x, const T prob_par) ABEL_NOEXCEPT {
            return (x == (long long) (1) ? prob_par : x == int64_t(0) ?
                                                      T(1) - prob_par : T(0));
        }

        template<typename T>
        ABEL_CONSTEXPR T bern_vals_check(const long long x, const T prob_par, const bool log_form) ABEL_NOEXCEPT {
            return (!bern_sanity_check(prob_par) ?
                    std::numeric_limits<T>::quiet_NaN() : abel::log_if(bern_compute(x, prob_par), log_form));
        }

    } //namespace stats_internal

    template<typename T>
    ABEL_CONSTEXPR return_t<T> pdf_bern(const long long x, const T prob_par, const bool log_form)
    ABEL_NOEXCEPT {
        return stats_internal::bern_vals_check(x, static_cast<return_t<T>>(prob_par), log_form);
    }

    namespace stats_internal {
        template<typename eT, typename T1, typename rT>
        ABEL_FORCE_INLINE void bern_vec(const eT *vals_in, const T1 prob_par,
                                        const bool log_form,
                                        rT *vals_out, const size_t num_elem) {

            for (size_t j = 0; j < num_elem; ++j) {
                vals_out[j] = abel::pdf_bern(vals_in[j], prob_par, log_form);
            }
        }
    } //namespace stats_internal

    template<typename eT, typename T1, typename rT>
    ABEL_FORCE_INLINE std::vector<rT> pdf_bern(const std::vector<eT> &x, const T1 prob_par, const bool log_form) {
        std::vector<rT> vec_out(x.size());
        stats_internal::bern_vec(x.data(), prob_par, log_form, vec_out.data(), x.size());
        return vec_out;
    }
} //namespace abel

#endif //ABEL_STATS_DENS_BERNOULLI_H_
