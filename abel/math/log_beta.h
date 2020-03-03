//
// Created by liyinbin on 2020/2/28.
//

#ifndef ABEL_MATH_LOG_BETA_H_
#define ABEL_MATH_LOG_BETA_H_

#include <abel/math/option.h>
#include <abel/math/log_gamma.h>

namespace abel {
    template<typename T1, typename T2>
    ABEL_CONSTEXPR common_return_t<T1,T2> lbeta(const T1 a, const T2 b) ABEL_NOEXCEPT {
        return( (lgamma(a) + lgamma(b)) - lgamma(a+b) );
    }

} //namespace abel

#endif //ABEL_MATH_LOG_BETA_H_
