// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com


#ifndef ABEL_META_INTERNAL_TYPE_COMPOUND_H_
#define ABEL_META_INTERNAL_TYPE_COMPOUND_H_

#include "abel/meta/internal/config.h"

namespace abel {

///////////////////////////////////////////////////////////////////////
// is_bounded_array
// part of c++20
///////////////////////////////////////////////////////////////////////

template<typename T>
struct is_bounded_array
        : public std::integral_constant<bool, std::extent<T>::value != 0> {
};

///////////////////////////////////////////////////////////////////////
// is_unbounded_array
// part of c++20
///////////////////////////////////////////////////////////////////////

template<typename T>
struct is_unbounded_array
        : public std::integral_constant<bool, std::is_array<T>::value && (std::extent<T>::value == 0)> {
};

}
#endif  // ABEL_META_INTERNAL_TYPE_COMPOUND_H_
