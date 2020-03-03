//
// Created by liyinbin on 2019/12/8.
//

#ifndef ABEL_MATH_ABS_DIFF_H_
#define ABEL_MATH_ABS_DIFF_H_

#include <abel/base/profile.h>
#include <abel/math/abs.h>
#include <vector>
#include <cstddef>


namespace abel {

    /**
     * @brief
     * @tparam T
     * @param a
     * @param b
     * @return
     */
    template<typename T>
    ABEL_FORCE_INLINE T abs_diff(const T &a, const T &b) {
        return a > b ? a - b : b - a;
    }

    template<typename T>
    ABEL_FORCE_INLINE T
    sum_abs_diff(const std::vector<T> &X, const std::vector<T> &Y) {
        T val_out = T(0);
        size_t n_elem = X.size(); // assumes dim(X) = dim(Y)

        for (size_t i = 0; i < n_elem; ++i) {
            val_out += abs(X[i] - Y[i]);
        }

        return val_out;
    }

} //namespace abel

#endif //ABEL_MATH_ABS_DIFF_H_
