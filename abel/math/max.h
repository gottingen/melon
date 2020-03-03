//
// Created by liyinbin on 2020/2/28.
//

#ifndef ABEL_MATH_MAX_H_
#define ABEL_MATH_MAX_H_

#include <abel/math/option.h>

namespace abel {

    template<typename T1, typename T2> ABEL_CONSTEXPR common_t<T1, T2> max(const T1 x, const T2 y) ABEL_NOEXCEPT {
        return (y < x ? x : y);
    }

}
#endif //ABEL_MATH_MAX_H_
