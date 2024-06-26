//
// Copyright (C) 2024 EA group inc.
// Author: Jeff.li lijippy@163.com
// All rights reserved.
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//
//


// Date: Tue Feb 25 23:43:39 CST 2014

// Provide functions to get/set bits of an integral array. These functions
// are not threadsafe because operations on different bits may modify a same
// integer.

#ifndef MUTIL_BIT_ARRAY_H
#define MUTIL_BIT_ARRAY_H

#include <stdint.h>

namespace mutil {

// Create an array with at least |nbit| bits. The array is not cleared.
inline uint64_t* bit_array_malloc(size_t nbit)
{
    if (!nbit) {
        return NULL;
    }
    return (uint64_t*)malloc((nbit + 63 ) / 64 * 8/*different from /8*/);
}

// Set bit 0 ~ nbit-1 of |array| to be 0
inline void bit_array_clear(uint64_t* array, size_t nbit)
{
    const size_t off = (nbit >> 6);
    memset(array, 0, off * 8);
    const size_t last = (off << 6);
    if (last != nbit) {
        array[off] &= ~((((uint64_t)1) << (nbit - last)) - 1);
    }
}

// Set i-th bit (from left, counting from 0) of |array| to be 1
inline void bit_array_set(uint64_t* array, size_t i)
{
    const size_t off = (i >> 6);
    array[off] |= (((uint64_t)1) << (i - (off << 6)));
}

// Set i-th bit (from left, counting from 0) of |array| to be 0
inline void bit_array_unset(uint64_t* array, size_t i)
{
    const size_t off = (i >> 6);
    array[off] &= ~(((uint64_t)1) << (i - (off << 6)));
}

// Get i-th bit (from left, counting from 0) of |array|
inline uint64_t bit_array_get(const uint64_t* array, size_t i)
{
    const size_t off = (i >> 6);
    return (array[off] & (((uint64_t)1) << (i - (off << 6))));
}

// Find index of first 1-bit from bit |begin| to |end| in |array|.
// Returns |end| if all bits are 0.
// This function is of O(nbit) complexity.
inline size_t bit_array_first1(const uint64_t* array, size_t begin, size_t end)
{
    size_t off1 = (begin >> 6);
    const size_t first = (off1 << 6);
    if (first != begin) {
        const uint64_t v =
            array[off1] & ~((((uint64_t)1) << (begin - first)) - 1);
        if (v) {
            return std::min(first + __builtin_ctzl(v), end);
        }
        ++off1;
    }
    
    const size_t off2 = (end >> 6);
    for (size_t i = off1; i < off2; ++i) {
        if (array[i]) {
            return i * 64 + __builtin_ctzl(array[i]);
        }
    }
    const size_t last = (off2 << 6);
    if (last != end && array[off2]) {
        return std::min(last + __builtin_ctzl(array[off2]), end);
    }
    return end;
}

}  // end namespace mutil

#endif  // MUTIL_BIT_ARRAY_H
