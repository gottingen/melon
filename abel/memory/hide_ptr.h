//
// Created by liyinbin on 2020/1/19.
//

#ifndef ABEL_MEMORY_HIDE_PTR_H_
#define ABEL_MEMORY_HIDE_PTR_H_

#include <cstdint>
#include <abel/base/profile.h>

namespace abel {


// Arbitrary value with high bits set. Xor'ing with it is unlikely
// to map one valid pointer to another valid pointer.
    constexpr uintptr_t hide_mask() {
        return (uintptr_t{0xF03A5F7BU} << (sizeof(uintptr_t) - 4) * 8) | 0xF03A5F7BU;
    }

// Hide a pointer from the leak checker. For internal use only.
// Differs from abel::ignore_leak(ptr) in that abel::ignore_leak(ptr) causes ptr
// and all objects reachable from ptr to be ignored by the leak checker.
    template<class T>
    ABEL_FORCE_INLINE uintptr_t hide_ptr(T *ptr) {
        return reinterpret_cast<uintptr_t>(ptr) ^ hide_mask();
    }

// Return a pointer that has been hidden from the leak checker.
// For internal use only.
    template<class T>
    ABEL_FORCE_INLINE T *unhide_ptr(uintptr_t hidden) {
        return reinterpret_cast<T *>(hidden ^ hide_mask());
    }


}
#endif //ABEL_MEMORY_HIDE_PTR_H_
