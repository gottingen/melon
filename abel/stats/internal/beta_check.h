//
// Created by liyinbin on 2020/3/3.
//

#ifndef ABEL_STATS_INTERNAL_BETA_CHECK_H_
#define ABEL_STATS_INTERNAL_BETA_CHECK_H_

#include <abel/math/all.h>

namespace abel {
    namespace stats_internal {

        template<typename T>
        ABEL_CONSTEXPR bool beta_sanity_check(const T a_par, const T b_par) ABEL_NOEXCEPT {
            return (abel::math_internal::any_nan(a_par, b_par) ? false : a_par < T(0) ?
                                                                         false : b_par < T(0) ?
                                                                                 false : true);
        }

    } //namespace stats_internal
} //namespace abel
#endif //ABEL_STATS_INTERNAL_BETA_CHECK_H_
