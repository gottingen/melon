//
// Created by liyinbin on 2021/4/5.
//

#ifndef ABEL_FIBER_THIS_FIBER_H_
#define ABEL_FIBER_THIS_FIBER_H_

#include "abel/chrono/clock.h"

namespace abel {

    // yield execution.
    //
    // If there's no other fiber is ready to run, the caller will be rescheduled
    // immediately.
    void fiber_yield();

    // Block calling fiber until `expires_at`.
    void fiber_sleep_until(const abel::time_point& expires_at);

    // Block calling fiber for `expires_in`.
    void fiber_sleep_for(const abel::duration& expires_in);


}  // namespace abel


#endif  // ABEL_FIBER_THIS_FIBER_H_
