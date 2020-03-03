//
// Created by liyinbin on 2020/3/3.
//

#ifndef ABEL_STATS_INTERNAL_BINOM_CHECK_H_
#define ABEL_STATS_INTERNAL_BINOM_CHECK_H_

#include <abel/math/all.h>

namespace abel {
    namespace stats_internal {


        template<typename T>
        ABEL_CONSTEXPR bool binom_sanity_check(const long long n_trials_par, const T prob_par) ABEL_NOEXCEPT {
            return (abel::math_internal::is_nan(prob_par) ? false : abel::math_internal::is_inf(prob_par) ?
                                                     false : n_trials_par < (long long)(0) ?
                                                             false : prob_par < T(0) ?
                                                                     false : prob_par > T(1) ?
                                                                             false : true);
        }
    } // namespace stats_internal
} //namespace abel
#endif //ABEL_STATS_INTERNAL_BINOM_CHECK_H_
