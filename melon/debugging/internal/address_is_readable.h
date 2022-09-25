
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************///

#ifndef MELON_DEBUGGING_INTERNAL_ADDRESS_IS_READABLE_H_
#define MELON_DEBUGGING_INTERNAL_ADDRESS_IS_READABLE_H_

#include "melon/base/profile.h"

namespace melon::debugging {

    namespace debugging_internal {

        // Return whether the byte at *addr is readable, without faulting.
        // Save and restores errno.
        bool address_is_readable(const void *addr);

    }  // namespace debugging_internal

}  // namespace melon::debugging

#endif  // MELON_DEBUGGING_INTERNAL_ADDRESS_IS_READABLE_H_
