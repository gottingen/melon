//
// Created by liyinbin on 2020/2/28.
//

#ifndef ABEL_BETA_H
#define ABEL_BETA_H

#include <abel/math/option.h>

namespace abel {

    template<typename T1, typename T2>
    ABEL_CONSTEXPR common_return_t<T1,T2> beta(const T1 a, const T2 b) ABEL_NOEXCEPT {
        return exp( lbeta(a,b) );
    }
}
#endif //ABEL_BETA_H
