
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/
#ifndef MELON_TIMES_INTERNAL_CHRONO_POSIX_TIME_H_
#define MELON_TIMES_INTERNAL_CHRONO_POSIX_TIME_H_

#include <sys/time.h>
#include <ctime>
#include <cstdint>
#include "melon/log/logging.h"

namespace melon::times_internal {

    static inline int64_t get_current_time_nanos_from_system() {
        const int64_t kNanosPerSecond = 1000 * 1000 * 1000;
        struct timespec ts;
        MELON_CHECK(clock_gettime(CLOCK_REALTIME, &ts) == 0)<<
               "Failed to read real-time clock.";
        return (int64_t{ts.tv_sec} * kNanosPerSecond +
                int64_t{ts.tv_nsec});
    }

}  // namespace melon::times_internal

#endif  // MELON_TIMES_INTERNAL_CHRONO_POSIX_TIME_H_
