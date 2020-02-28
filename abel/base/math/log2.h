//
// Created by liyinbin on 2019/12/8.
//

#ifndef ABEL_BASE_MATH_LOG2_H_
#define ABEL_BASE_MATH_LOG2_H_

#include <abel/base/profile.h>
#include <cmath>

namespace abel {

// integer_log2_floor()

//! calculate the log2 floor of an integer type
    template<typename IntegerType>
    static ABEL_FORCE_INLINE unsigned integer_log2_floor_template(IntegerType i) {
        unsigned p = 0;
        while (i >= 65536)
            i >>= 16, p += 16;
        while (i >= 256)
            i >>= 8, p += 8;
        while (i >>= 1)
            ++p;
        return p;
    }

// integer_log2_floor()

#if defined(ABEL_COMPILER_GNUC) || defined(ABEL_COMPILER_CLANG)

//! calculate the log2 floor of an integer type
    static ABEL_FORCE_INLINE unsigned integer_log2_floor(int i) {
        if (i == 0)
            return 0;
        return 8 * sizeof(int) - 1 - __builtin_clz(i);
    }

//! calculate the log2 floor of an integer type
    static ABEL_FORCE_INLINE unsigned integer_log2_floor(unsigned int i) {
        if (i == 0)
            return 0;
        return 8 * sizeof(unsigned) - 1 - __builtin_clz(i);
    }

//! calculate the log2 floor of an integer type
    static ABEL_FORCE_INLINE unsigned integer_log2_floor(long i) {
        if (i == 0)
            return 0;
        return 8 * sizeof(long) - 1 - __builtin_clzl(i);
    }

//! calculate the log2 floor of an integer type
    static ABEL_FORCE_INLINE unsigned integer_log2_floor(unsigned long i) {
        if (i == 0)
            return 0;
        return 8 * sizeof(unsigned long) - 1 - __builtin_clzl(i);
    }

//! calculate the log2 floor of an integer type
    static ABEL_FORCE_INLINE unsigned integer_log2_floor(long long i) {
        if (i == 0)
            return 0;
        return 8 * sizeof(long long) - 1 - __builtin_clzll(i);
    }

//! calculate the log2 floor of an integer type
    static ABEL_FORCE_INLINE unsigned integer_log2_floor(unsigned long long i) {
        if (i == 0)
            return 0;
        return 8 * sizeof(unsigned long long) - 1 - __builtin_clzll(i);
    }

#else

    //! calculate the log2 floor of an integer type
    static ABEL_FORCE_INLINE unsigned integer_log2_floor (int i) {
        return integer_log2_floor_template(i);
    }

    //! calculate the log2 floor of an integer type
    static ABEL_FORCE_INLINE unsigned integer_log2_floor (unsigned int i) {
        return integer_log2_floor_template(i);
    }

    //! calculate the log2 floor of an integer type
    static ABEL_FORCE_INLINE unsigned integer_log2_floor (long i) {
        return integer_log2_floor_template(i);
    }

    //! calculate the log2 floor of an integer type
    static ABEL_FORCE_INLINE unsigned integer_log2_floor (unsigned long i) {
        return integer_log2_floor_template(i);
    }

    //! calculate the log2 floor of an integer type
    static ABEL_FORCE_INLINE unsigned integer_log2_floor (long long i) {
        return integer_log2_floor_template(i);
    }

    //! calculate the log2 floor of an integer type
    static ABEL_FORCE_INLINE unsigned integer_log2_floor (unsigned long long i) {
        return integer_log2_floor_template(i);
    }

#endif

/******************************************************************************/
// integer_log2_ceil()

//! calculate the log2 floor of an integer type
    static ABEL_FORCE_INLINE unsigned integer_log2_ceil(int i) {
        if (i <= 1)
            return 0;
        return integer_log2_floor(i - 1) + 1;
    }

//! calculate the log2 floor of an integer type
    static ABEL_FORCE_INLINE unsigned integer_log2_ceil(unsigned int i) {
        if (i <= 1)
            return 0;
        return integer_log2_floor(i - 1) + 1;
    }

//! calculate the log2 floor of an integer type
    static ABEL_FORCE_INLINE unsigned integer_log2_ceil(long i) {
        if (i <= 1)
            return 0;
        return integer_log2_floor(i - 1) + 1;
    }

//! calculate the log2 floor of an integer type
    static ABEL_FORCE_INLINE unsigned integer_log2_ceil(unsigned long i) {
        if (i <= 1)
            return 0;
        return integer_log2_floor(i - 1) + 1;
    }

//! calculate the log2 floor of an integer type
    static ABEL_FORCE_INLINE unsigned integer_log2_ceil(long long i) {
        if (i <= 1)
            return 0;
        return integer_log2_floor(i - 1) + 1;
    }

//! calculate the log2 floor of an integer type
    static ABEL_FORCE_INLINE unsigned integer_log2_ceil(unsigned long long i) {
        if (i <= 1)
            return 0;
        return integer_log2_floor(i - 1) + 1;
    }

    ABEL_FORCE_INLINE double stirling_log_factorial(double n) {
        assert(n >= 1);
        // Using Stirling's approximation.
        constexpr double kLog2PI = 1.83787706640934548356;
        const double logn = std::log(n);
        const double ninv = 1.0 / static_cast<double>(n);
        return n * logn - n + 0.5 * (kLog2PI + logn) + (1.0 / 12.0) * ninv -
               (1.0 / 360.0) * ninv * ninv * ninv;
    }

} //namespace abel

#endif //ABEL_BASE_MATH_LOG2_H_
