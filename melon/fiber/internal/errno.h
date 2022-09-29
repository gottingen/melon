
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef MELON_FIBER_INTERNAL_ERRNO_H_
#define MELON_FIBER_INTERNAL_ERRNO_H_

#include <errno.h>                    // errno
#include "melon/base/errno.h"                // melon_error(),

__BEGIN_DECLS

extern int *fiber_errno_location();

#ifdef errno
#undef errno
#define errno *fiber_errno_location()
#endif

// List errno used throughout fiber
extern const int ESTOP;

__END_DECLS

#endif  // MELON_FIBER_INTERNAL_ERRNO_H_
