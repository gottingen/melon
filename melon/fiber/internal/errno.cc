
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#include "melon/fiber/internal/errno.h"
#include "melon/base/profile.h"

// Define errno in fiber/internal/errno.h
extern const int ESTOP = -20;

MELON_REGISTER_ERRNO(ESTOP, "The structure is stopping")

extern "C" {

#if defined(MELON_PLATFORM_LINUX)

extern int *__errno_location() __attribute__((__const__));

int *fiber_errno_location() {
    return __errno_location();
}
#elif defined(MELON_PLATFORM_OSX)

extern int * __error(void);

int *fiber_errno_location() {
    return __error();
}
#endif

}  // extern "C"
