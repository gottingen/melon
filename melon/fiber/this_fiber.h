
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/



#ifndef MELON_FIBER_THIS_FIBER_H_
#define MELON_FIBER_THIS_FIBER_H_

#include <cstdint>

namespace melon {

    int fiber_yield();

    // expires_at_us should based microseconds melon::get_current_time_micros();
    int fiber_sleep_until(const int64_t& expires_at_us);

    int fiber_sleep_for(const int64_t& expires_in_us);

    uint64_t get_fiber_id();

}  // namespace melon

#endif // MELON_FIBER_THIS_FIBER_H_

