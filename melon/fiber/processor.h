//
// Copyright (C) 2024 EA group inc.
// Author: Jeff.li lijippy@163.com
// All rights reserved.
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//
//


#ifndef MELON_FIBER_PROCESSOR_H_
#define MELON_FIBER_PROCESSOR_H_

#include <melon/base/build_config.h>

// Pause instruction to prevent excess processor bus usage, only works in GCC
# ifndef cpu_relax
#if defined(ARCH_CPU_ARM_FAMILY)
# define cpu_relax() asm volatile("yield\n": : :"memory")
#elif defined(ARCH_CPU_LOONGARCH64_FAMILY)
# define cpu_relax() asm volatile("nop\n": : :"memory");
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

#endif // MELON_FIBER_PROCESSOR_H_
