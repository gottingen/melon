//
// Created by liyinbin on 2020/2/1.
//

#ifndef ABEL_BASE_MATH_ALIGN_H_
#define ABEL_BASE_MATH_ALIGN_H_

#include <abel/base/profile.h>
#include <cstdint>
#include <cstdlib>

namespace able {

    template<typename T>
    ABEL_FORCE_INLINE constexpr
    T align_up(T v, T align) {
        return (v + align - 1) & ~(align - 1);
    }

    template<typename T>
    ABEL_FORCE_INLINE constexpr
    T *align_up(T *v, size_t align) {
        static_assert(sizeof(T) == 1, "align byte pointers only");
        return reinterpret_cast<T *>(align_up(reinterpret_cast<uintptr_t>(v), align));
    }

    template<typename T>
    ABEL_FORCE_INLINE constexpr
    T align_down(T v, T align) {
        return v & ~(align - 1);
    }

    template<typename T>
    ABEL_FORCE_INLINE constexpr
    T *align_down(T *v, size_t align) {
        static_assert(sizeof(T) == 1, "align byte pointers only");
        return reinterpret_cast<T *>(align_down(reinterpret_cast<uintptr_t>(v), align));
    }
} //namespace abel

#endif //ABEL_BASE_MATH_ALIGN_H_
