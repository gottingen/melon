//
// Created by liyinbin on 2020/1/29.
//

#ifndef ABEL_CHRONO_INTERNAL_CHRONO_TIME_H_
#define ABEL_CHRONO_INTERNAL_CHRONO_TIME_H_

namespace abel {

namespace chrono_internal {

static int64_t get_current_time_nanos_from_system () {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::system_clock::now() -
            std::chrono::system_clock::from_time_t(0)).count();
}

} //namespace chrono_internal
} //namespace alel

#endif //ABEL_CHRONO_INTERNAL_CHRONO_TIME_H_
