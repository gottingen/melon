
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/


#include "melon/fiber/internal/fiber.h"

namespace melon {

    int fiber_getconcurrency(void) {
        return ::fiber_getconcurrency();
    }

    int fiber_setconcurrency(int num) {
        return ::fiber_setconcurrency(num);
    }

}  // namespace melon
