// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com
//

#ifndef ABEL_DEBUGGING_INTERNAL_ADDRESS_IS_READABLE_H_
#define ABEL_DEBUGGING_INTERNAL_ADDRESS_IS_READABLE_H_

#include "abel/base/profile.h"

namespace abel {

namespace debugging_internal {

// Return whether the byte at *addr is readable, without faulting.
// Save and restores errno.
bool address_is_readable(const void *addr);

}  // namespace debugging_internal

}  // namespace abel

#endif  // ABEL_DEBUGGING_INTERNAL_ADDRESS_IS_READABLE_H_
