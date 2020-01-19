//
// Created by liyinbin on 2019/12/8.
//

#ifndef ABEL_BASE_MATH_SIGN_H_
#define ABEL_BASE_MATH_SIGN_H_

#include <abel/base/profile.h>

namespace abel {

    template<typename T>
    int sgn (const T &val) {
        return (T(0) < val) - (val < T(0));
    }

} //namespace abel

#endif //ABEL_BASE_MATH_SIGN_H_
