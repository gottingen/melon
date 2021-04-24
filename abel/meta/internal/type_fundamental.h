// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com


#ifndef ABEL_META_INTERANL_TYPE_FUNDAMENTAL_H_
#define ABEL_META_INTERANL_TYPE_FUNDAMENTAL_H_

#include "abel/meta/internal/config.h"

namespace abel {

template<typename T>
struct is_smart_ptr : std::false_type {
};

template<typename T>
struct is_smart_ptr<std::unique_ptr<T>> : std::true_type {
};

}  // namespace abel

#endif  // ABEL_META_INTERANL_TYPE_FUNDAMENTAL_H_
