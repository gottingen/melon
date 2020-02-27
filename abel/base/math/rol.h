//
// Created by liyinbin on 2019/12/8.
//

#ifndef ABEL_BASE_MATH_ROL_H_
#define ABEL_BASE_MATH_ROL_H_

#include <abel/base/profile.h>
#include <cstdint>
#include <cstdlib>

namespace abel {

// rol32() - rotate bits left in 32-bit integers

//! rol32 - generic implementation
    static ABEL_FORCE_INLINE uint32_t rol32_generic(const uint32_t &x, int i) {
        return (x << static_cast<uint32_t>(i & 31)) |
               (x >> static_cast<uint32_t>((32 - (i & 31)) & 31));
    }

#if (defined(__GNUC__) || defined(__clang__)) && (defined(__i386__) || defined(__x86_64__))

//! rol32 - gcc/clang assembler
    static ABEL_FORCE_INLINE uint32_t rol32(const uint32_t &x, int i) {
        uint32_t x1 = x;
        asm ("roll %%cl,%0" : "=r" (x1) : "0" (x1), "c" (i));
        return x1;
    }

#elif defined(_MSC_VER)

    //! rol32 - MSVC intrinsic
    static ABEL_FORCE_INLINE uint32_t rol32(const uint32_t& x, int i) {
        return _rotl(x, i);
    }

#else

    //! rol32 - generic
    static ABEL_FORCE_INLINE uint32_t rol32(const uint32_t& x, int i) {
        return rol32_generic(x, i);
    }

#endif


// rol64() - rotate bits left in 64-bit integers

//! rol64 - generic implementation
    static ABEL_FORCE_INLINE uint64_t rol64_generic(const uint64_t &x, int i) {
        return (x << static_cast<uint64_t>(i & 63)) |
               (x >> static_cast<uint64_t>((64 - (i & 63)) & 63));
    }

#if (defined(__GNUC__) || defined(__clang__)) && defined(__x86_64__)

//! rol64 - gcc/clang assembler
    static ABEL_FORCE_INLINE uint64_t rol64(const uint64_t &x, int i) {
        uint64_t x1 = x;
        asm ("rolq %%cl,%0" : "=r" (x1) : "0" (x1), "c" (i));
        return x1;
    }

#else

    //! rol64 - generic
    static ABEL_FORCE_INLINE uint64_t rol64(const uint64_t& x, int i) {
        return rol64_generic(x, i);
    }

#endif

} //namespace abel

#endif //ABEL_BASE_MATH_ROL_H_
