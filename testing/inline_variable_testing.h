// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com


#ifndef TEST_TESTING_INLINE_VARIABLE_TESTING_H_
#define TEST_TESTING_INLINE_VARIABLE_TESTING_H_

#include "abel/utility/internal/inline_variable.h"
#include "abel/base/profile.h"

namespace abel {

namespace inline_variable_testing_internal {

struct Foo {
    int value = 5;
};

ABEL_INTERNAL_INLINE_CONSTEXPR(Foo, inline_variable_foo, {});
ABEL_INTERNAL_INLINE_CONSTEXPR(Foo, other_inline_variable_foo, {});

ABEL_INTERNAL_INLINE_CONSTEXPR(int, inline_variable_int, 5);
ABEL_INTERNAL_INLINE_CONSTEXPR(int, other_inline_variable_int, 5);

ABEL_INTERNAL_INLINE_CONSTEXPR(void(*)
                                       (), inline_variable_fun_ptr, nullptr);

const Foo &get_foo_a();

const Foo &get_foo_b();

const int &get_int_a();

const int &get_int_b();

}  // namespace inline_variable_testing_internal

}  // namespace abel

#endif  // TEST_TESTING_INLINE_VARIABLE_TESTING_H_
