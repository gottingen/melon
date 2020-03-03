//
// Created by liyinbin on 2020/3/3.
//

#ifndef ABEL_STATS_INTERNAL_BERN_CHECK_H_
#define ABEL_STATS_INTERNAL_BERN_CHECK_H_

#include <abel/math/all.h>

namespace abel {
    namespace stats_internal {
        template<typename T>
        ABEL_CONSTEXPR bool bern_sanity_check(const T prob_par) ABEL_NOEXCEPT {
            return (abel::math_internal::is_nan(prob_par) ? false : abel::math_internal::is_inf(prob_par) ?
                                                                    false : prob_par < T(0) ?
                                                                            false : prob_par > T(1) ?
                                                                                    false : true
            );
        }
    } //namespace stats_internal
} //namespace abel

#endif //ABEL_STATS_INTERNAL_BERN_CHECK_H_
