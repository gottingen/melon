
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef MELON_FIBER_INTERNAL_INTERRUPT_PTHREAD_H_
#define MELON_FIBER_INTERNAL_INTERRUPT_PTHREAD_H_

#include <pthread.h>

namespace melon::fiber_internal {

    // Make blocking ops in the pthread returns -1 and EINTR.
    // Returns what pthread_kill returns.
    int interrupt_pthread(pthread_t th);

}  // namespace melon::fiber_internal

#endif // MELON_FIBER_INTERNAL_INTERRUPT_PTHREAD_H_
