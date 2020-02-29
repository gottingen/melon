//
// Created by liyinbin on 2020/2/28.
//

#ifndef ABEL_LOG_BETA_H
#define ABEL_LOG_BETA_H

#include <abel/math/option.h>
#include <abel/math/log_gamma.h>

namespace abel {
    template<typename T1, typename T2>
    ABEL_CONSTEXPR common_return_t<T1,T2> lbeta(const T1 a, const T2 b) ABEL_NOEXCEPT {
        return( (lgamma(a) + lgamma(b)) - lgamma(a+b) );
    }

} //namespace abel

#endif //ABEL_LOG_BETA_H
