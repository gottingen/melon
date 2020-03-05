//
// Created by liyinbin on 2020/2/28.
//

#ifndef ABEL_MATH_LOG_H_
#define ABEL_MATH_LOG_H_

#include <abel/base/profile.h>
namespace abel {


// log2_floor()

//! calculate the log2 floor of an integer type
    template<typename IntegerType>
    ABEL_CONSTEXPR unsigned log2_floor(IntegerType n) {
        return (n <= 1) ? 0 : 1 + log2_floor(n / 2);
    }

/******************************************************************************/
// log2_ceil()

//! calculate the log2 floor of an integer type
    template<typename IntegerType>
    ABEL_CONSTEXPR unsigned log2_ceil(IntegerType n) {
        return (n <= 1) ? 0 : log2_floor(n - 1) + 1;
    }

    ABEL_FORCE_INLINE double stirling_log_factorial(double n) {
        assert(n >= 1);
        // Using Stirling's approximation.
        constexpr double kLog2PI = 1.83787706640934548356;
        const double logn = log(n);
        const double ninv = 1.0 / static_cast<double>(n);
        return n * logn - n + 0.5 * (kLog2PI + logn) + (1.0 / 12.0) * ninv -
               (1.0 / 360.0) * ninv * ninv * ninv;
    }


} //namespace abel

#endif //ABEL_MATH_LOG_H_
