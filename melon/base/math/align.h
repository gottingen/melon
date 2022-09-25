
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef MELON_BASE_MATH_ALIGN_H_
#define MELON_BASE_MATH_ALIGN_H_

#include <cstdint>
#include <cstdlib>
#include "melon/base/profile.h"

namespace melon::base {

    template<typename T>

    constexpr T align_up(T v, T align) {
        return (v + align - 1) & ~(align - 1);
    }

    template<typename T>
    constexpr T *align_up(T *v, size_t align) {
        static_assert(sizeof(T) == 1, "align byte pointers only");
        return reinterpret_cast<T *>(align_up(reinterpret_cast<uintptr_t>(v), align));
    }

    template<typename T>
    constexpr T align_down(T v, T align) {
        return v & ~(align - 1);
    }

    template<typename T>
    constexpr T *align_down(T *v, size_t align) {
        static_assert(sizeof(T) == 1, "align byte pointers only");
        return reinterpret_cast<T *>(align_down(reinterpret_cast<uintptr_t>(v), align));
    }
}  // namespace melon::base

#endif  // MELON_BASE_MATH_ALIGN_H_
