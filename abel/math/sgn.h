//
// Created by liyinbin on 2020/2/28.
//

#ifndef ABEL_MATH_SGN_H_
#define ABEL_MATH_SGN_H_

namespace abel {


    template<typename T>
    ABEL_CONSTEXPR int sgn(const T x) ABEL_NOEXCEPT {
        return (x > T(0) ? 1 : x < T(0) ? -1 : 0);
    }
}
#endif //ABEL_MATH_SGN_H_
