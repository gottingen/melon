//
// Created by liyinbin on 2019/12/8.
//

#ifndef ABEL_BASE_MATH_POP_COUNT_H_
#define ABEL_BASE_MATH_POP_COUNT_H_

#include <abel/base/profile.h>
#include <cstdint>
#include <cstdlib>

namespace abel {

// pop_count() - count one bits

//! pop_count (count one bits) - generic SWAR implementation
    static ABEL_FORCE_INLINE unsigned pop_count_generic8(uint8_t x) {
        x = x - ((x >> 1) & 0x55);
        x = (x & 0x33) + ((x >> 2) & 0x33);
        return static_cast<uint8_t>((x + (x >> 4)) & 0x0F);
    }

//! pop_count (count one bits) - generic SWAR implementation
    static ABEL_FORCE_INLINE unsigned pop_count_generic16(uint16_t x) {
        x = x - ((x >> 1) & 0x5555);
        x = (x & 0x3333) + ((x >> 2) & 0x3333);
        return static_cast<uint16_t>(((x + (x >> 4)) & 0x0F0F) * 0x0101) >> 8;
    }

//! pop_count (count one bits) -
//! generic SWAR implementation from https://stackoverflow.com/questions/109023
    static ABEL_FORCE_INLINE unsigned pop_count_generic32(uint32_t x) {
        x = x - ((x >> 1) & 0x55555555);
        x = (x & 0x33333333) + ((x >> 2) & 0x33333333);
        return (((x + (x >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;
    }

//! pop_count (count one bits) - generic SWAR implementation
    static ABEL_FORCE_INLINE unsigned pop_count_generic64(uint64_t x) {
        x = x - ((x >> 1) & 0x5555555555555555);
        x = (x & 0x3333333333333333) + ((x >> 2) & 0x3333333333333333);
        return (((x + (x >> 4)) & 0x0F0F0F0F0F0F0F0F) * 0x0101010101010101) >> 56;
    }

#if defined(ABEL_COMPILER_CLANG) || defined(ABEL_COMPILER_GNUC)

//! pop_count (count one bits)
    static ABEL_FORCE_INLINE unsigned pop_count(unsigned i) {
        return static_cast<unsigned>(__builtin_popcount(i));
    }

//! pop_count (count one bits)
    static ABEL_FORCE_INLINE unsigned pop_count(int i) {
        return pop_count(static_cast<unsigned>(i));
    }

//! pop_count (count one bits)
    static ABEL_FORCE_INLINE unsigned pop_count(unsigned long i) {
        return static_cast<unsigned>(__builtin_popcountl(i));
    }

//! pop_count (count one bits)
    static ABEL_FORCE_INLINE unsigned pop_count(long i) {
        return pop_count(static_cast<unsigned long>(i));
    }

//! pop_count (count one bits)
    static ABEL_FORCE_INLINE unsigned pop_count(unsigned long long i) {
        return static_cast<unsigned>(__builtin_popcountll(i));
    }

//! pop_count (count one bits)
    static ABEL_FORCE_INLINE unsigned pop_count(long long i) {
        return pop_count(static_cast<unsigned long long>(i));
    }

#else

    //! pop_count (count one bits)
    template<typename Integral>
    ABEL_FORCE_INLINE unsigned pop_count (Integral i) {
        if (sizeof(i) <= sizeof(uint8_t))
            return pop_count_generic8(i);
        else if (sizeof(i) <= sizeof(uint16_t))
            return pop_count_generic16(i);
        else if (sizeof(i) <= sizeof(uint32_t))
            return pop_count_generic32(i);
        else if (sizeof(i) <= sizeof(uint64_t))
            return pop_count_generic64(i);
        else
            abort();
    }

#endif

// pop_count range

    static ABEL_FORCE_INLINE
    size_t pop_count(const void *data, size_t size) {
        const uint8_t *begin = reinterpret_cast<const uint8_t *>(data);
        const uint8_t *end = begin + size;
        size_t total = 0;
        while (begin + 7 < end) {
            total += pop_count(*reinterpret_cast<const uint64_t *>(begin));
            begin += 8;
        }
        if (begin + 3 < end) {
            total += pop_count(*reinterpret_cast<const uint32_t *>(begin));
            begin += 4;
        }
        while (begin < end) {
            total += pop_count(*begin++);
        }
        return total;
    }

} //namespace abel

#endif //ABEL_BASE_MATH_POP_COUNT_H_
