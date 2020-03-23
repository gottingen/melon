//
// Created by liyinbin on 2020/3/22.
//

#ifndef ABEL_ASL_INTERNAL_TYPE_COMPOUND_H_
#define ABEL_ASL_INTERNAL_TYPE_COMPOUND_H_

#include <abel/asl/internal/config.h>

namespace abel {

    ///////////////////////////////////////////////////////////////////////
    // is_bounded_array
    // part of c++20
    ///////////////////////////////////////////////////////////////////////

    template<typename T>
    struct is_bounded_array
            : public std::integral_constant<bool, std::extent<T>::value != 0> {};

    ///////////////////////////////////////////////////////////////////////
    // is_unbounded_array
    // part of c++20
    ///////////////////////////////////////////////////////////////////////

    template<typename T>
    struct is_unbounded_array
            : public std::integral_constant<bool, std::is_array<T>::value && (std::extent<T>::value == 0)> {};

}
#endif //ABEL_ASL_INTERNAL_TYPE_COMPOUND_H_
