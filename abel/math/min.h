//
// Created by liyinbin on 2020/2/28.
//

#ifndef ABEL_MIN_H
#define ABEL_MIN_H

#include <abel/math/option.h>

namespace abel {

    template<typename T1, typename T2>
    ABEL_CONSTEXPR common_t<T1, T2> min(const T1 x, const T2 y) ABEL_NOEXCEPT {
        return (y > x ? x : y);
    }
} //namespace abel

#endif //ABEL_MIN_H
