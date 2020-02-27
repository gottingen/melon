//
// Created by liyinbin on 2019/12/8.
//

#ifndef ABEL_BASE_MATH_CLZ_H_
#define ABEL_BASE_MATH_CLZ_H_

#include <abel/base/profile.h>

namespace abel {

    template<typename Integral>
    ABEL_FORCE_INLINE unsigned clz_template(Integral x) {
        if (x == 0)
            return 8 * sizeof(x);
        unsigned r = 0;
        while ((x & (static_cast<Integral>(1) << (8 * sizeof(x) - 1))) == 0)
            x <<= 1, ++r;
        return r;
    }

    template<typename Integral>
    ABEL_FORCE_INLINE unsigned clz_non_template(Integral x) {
        auto lz = clz_template(x);
        return sizeof(x) * 8 - 1 - lz;
    }


    template<typename Integral>
    ABEL_FORCE_INLINE unsigned count_leading_zeros(Integral x);

#if defined(ABEL_COMPILER_GNUC) || defined(ABEL_COMPILER_CLANG)

    template<>
    ABEL_FORCE_INLINE unsigned count_leading_zeros<unsigned>(unsigned x) {
        if (x == 0)
            return 8 * sizeof(x);
        return static_cast<unsigned>(__builtin_clz(x));
    }

//! count_leading_zeros (count leading zeros)
    template<>
    ABEL_FORCE_INLINE unsigned count_leading_zeros<int>(int i) {
        return count_leading_zeros(static_cast<unsigned>(i));
    }

//! count_leading_zeros (count leading zeros)
    template<>
    ABEL_FORCE_INLINE unsigned count_leading_zeros<unsigned long>(unsigned long i) {
        if (i == 0)
            return 8 * sizeof(i);
        return static_cast<unsigned>(__builtin_clzl(i));
    }

//! count_leading_zeros (count leading zeros)
    template<>
    ABEL_FORCE_INLINE unsigned count_leading_zeros<long>(long i) {
        return count_leading_zeros(static_cast<unsigned long>(i));
    }

//! count_leading_zeros (count leading zeros)
    template<>
    ABEL_FORCE_INLINE unsigned count_leading_zeros<unsigned long long>(unsigned long long i) {
        if (i == 0)
            return 8 * sizeof(i);
        return static_cast<unsigned>(__builtin_clzll(i));
    }

//! count_leading_zeros (count leading zeros)
    template<>
    ABEL_FORCE_INLINE unsigned count_leading_zeros<long long>(long long i) {
        return count_leading_zeros(static_cast<unsigned long long>(i));
    }

#else

    //! count_leading_zeros (count leading zeros)
    template<>
    ABEL_FORCE_INLINE unsigned count_leading_zeros<int> (int i) {
        return clz_template(i);
    }

    //! count_leading_zeros (count leading zeros)
    template<>
    ABEL_FORCE_INLINE unsigned count_leading_zeros<unsigned> (unsigned i) {
        return clz_template(i);
    }

    //! count_leading_zeros (count leading zeros)
    template<>
    ABEL_FORCE_INLINE unsigned count_leading_zeros<long> (long i) {
        return clz_template(i);
    }

    //! count_leading_zeros (count leading zeros)
    template<>
    ABEL_FORCE_INLINE unsigned count_leading_zeros<unsigned long> (unsigned long i) {
        return clz_template(i);
    }

    //! count_leading_zeros (count leading zeros)
    template<>
    ABEL_FORCE_INLINE unsigned count_leading_zeros<long long> (long long i) {
        return clz_template(i);
    }

    //! count_leading_zeros (count leading zeros)
    template<>
    ABEL_FORCE_INLINE unsigned count_leading_zeros<unsigned long long> (unsigned long long i) {
        return clz_template(i);
    }

#endif

    template<typename Integral>
    ABEL_FORCE_INLINE unsigned count_leading_non_zeros(Integral x) {
        auto lz = count_leading_zeros(x);
        return sizeof(x) * 8 - 1 - lz;
    }

} //namespace abel

#endif //ABEL_BASE_MATH_CLZ_H_
