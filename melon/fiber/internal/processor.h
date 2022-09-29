
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef MELON_FIBER_INTERNAL_PROCESSOR_H_
#define MELON_FIBER_INTERNAL_PROCESSOR_H_

#include "melon/base/profile.h"

// Pause instruction to prevent excess processor bus usage, only works in GCC
# ifndef cpu_relax
#if defined(MELON_PROCESSOR_ARM)
# define cpu_relax() asm volatile("yield\n": : :"memory")
#else
# define cpu_relax() asm volatile("pause\n": : :"memory")
#endif
# endif

// Compile read-write barrier
# ifndef barrier
# define barrier() asm volatile("": : :"memory")
# endif


# define BT_LOOP_WHEN(expr, num_spins)                                  \
    do {                                                                \
        /*sched_yield may change errno*/                                \
        const int saved_errno = errno;                                  \
        for (int cnt = 0, saved_nspin = (num_spins); (expr); ++cnt) {   \
            if (cnt < saved_nspin) {                                    \
                cpu_relax();                                            \
            } else {                                                    \
                sched_yield();                                          \
            }                                                           \
        }                                                               \
        errno = saved_errno;                                            \
    } while (0)

#endif // MELON_FIBER_INTERNAL_PROCESSOR_H_
