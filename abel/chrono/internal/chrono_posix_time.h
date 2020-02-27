//
// Created by liyinbin on 2020/1/29.
//

#ifndef ABEL_CHRONO_INTERNAL_CHRONO_POSIX_TIME_H_
#define ABEL_CHRONO_INTERNAL_CHRONO_POSIX_TIME_H_

#include <abel/chrono/clock.h>
#include <sys/time.h>
#include <ctime>
#include <cstdint>
#include <abel/log/raw_logging.h>

namespace abel {

    namespace chrono_internal {

        static inline int64_t get_current_time_nanos_from_system() {
            const int64_t kNanosPerSecond = 1000 * 1000 * 1000;
            struct timespec ts;
            ABEL_RAW_CHECK(clock_gettime(CLOCK_REALTIME, &ts) == 0,
                           "Failed to read real-time clock.");
            return (int64_t{ts.tv_sec} * kNanosPerSecond +
                    int64_t{ts.tv_nsec});
        }

    } //namespace chrono_internal
} //namespace alel

#endif //ABEL_CHRONO_INTERNAL_CHRONO_POSIX_TIME_H_
