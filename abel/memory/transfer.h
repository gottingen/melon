//
// Created by liyinbin on 2020/1/31.
//

#ifndef ABEL_MEMORY_TRANSFER_H_
#define ABEL_MEMORY_TRANSFER_H_

#include <type_traits>
#include <utility>
#include <abel/base/profile.h>

namespace abel {

    template<typename T, typename Alloc>
    ABEL_FORCE_INLINE void
    transfer(Alloc &a, T *from, T *to,
             typename std::enable_if<std::is_nothrow_move_constructible<T>::value>::type * = nullptr) {
        a.construct(to, std::move(*from));
        a.destroy(from);
    }

    template<typename T, typename Alloc>
    inline
    void
    transfer_undo(Alloc &a, T *from, T *to,
                  typename std::enable_if<std::is_nothrow_move_constructible<T>::value>::type * = nullptr) {
    }

    template<typename T, typename Alloc>
    inline
    void
    transfer(Alloc &a, T *from, T *to,
             typename std::enable_if<!std::is_nothrow_move_constructible<T>::value>::type * = nullptr) {
        a.construct(to, *from);
    }

    template<typename T, typename Alloc>
    inline
    void
    transfer_undo(Alloc &a, T *from, T *to,
                  typename std::enable_if<!std::is_nothrow_move_constructible<T>::value>::type * = nullptr) {
        a.destroy(from);
    }

} //namespace abel
#endif //ABEL_MEMORY_TRANSFER_H_
