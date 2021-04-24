// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com

#ifndef ABEL_CHRONO_INTERNAL_CHRONO_POSIX_TIME_H_
#define ABEL_CHRONO_INTERNAL_CHRONO_POSIX_TIME_H_

#include <sys/time.h>
#include <ctime>
#include <cstdint>
#include "abel/log/logging.h"
#include "abel/chrono/clock.h"

namespace abel {

namespace chrono_internal {

static inline int64_t get_current_time_nanos_from_system() {
    const int64_t kNanosPerSecond = 1000 * 1000 * 1000;
    struct timespec ts;
    DCHECK_MSG(clock_gettime(CLOCK_REALTIME, &ts) == 0,
           "Failed to read real-time clock.");
    return (int64_t{ts.tv_sec} * kNanosPerSecond +
            int64_t{ts.tv_nsec});
}

}  // namespace chrono_internal
}  // namespace abel

#endif  // ABEL_CHRONO_INTERNAL_CHRONO_POSIX_TIME_H_
