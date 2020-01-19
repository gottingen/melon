//
// Created by liyinbin on 2019/12/8.
//

#ifndef ABEL_BASE_MATH_ABS_DIFF_H_
#define ABEL_BASE_MATH_ABS_DIFF_H_
#include <abel/base/profile.h>

namespace abel {

/**
 * @brief
 * @tparam T
 * @param a
 * @param b
 * @return
 */
template<typename T>
ABEL_FORCE_INLINE T abs_diff (const T &a, const T &b) {
    return a > b ? a - b : b - a;
}

} //namespace abel

#endif //ABEL_BASE_MATH_ABS_DIFF_H_
