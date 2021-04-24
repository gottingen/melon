// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com

#ifndef ABEL_CHRONO_TIMER_H_
#define ABEL_CHRONO_TIMER_H_

#include <time.h>
#include <algorithm>
#include <limits>
#include "abel/log/logging.h"
#include "abel/chrono/clock.h"

namespace abel {

class timer {
  public:
    // A timeout that should expire at <t>.  Any value, in the full
    // infinite_past() to infinite_future() range, is valid here and will be
    // respected.
    explicit timer(abel::time_point t) : _ns(make_ns(t)) {}

    // No timeout.
    timer() : _ns(0) {}

    // A more explicit factory for those who prefer it.  Equivalent to {}.
    static timer never() { return {}; }

    // We explicitly do not support other custom formats: timespec, int64_t nanos.
    // Unify on this and abel::time_point, please.

    bool has_timeout() const { return _ns != 0; }

    // Convert to parameter for sem_timedwait/futex/similar.  Only for approved
    // users.  Do not call if !has_timeout.
    struct timespec make_abs_timespec() {
        int64_t n = _ns;
        static const int64_t kNanosPerSecond = 1000 * 1000 * 1000;
        if (n == 0) {
            DLOG_ERROR("Tried to create a timespec from a non-timeout; never do this.");
            // But we'll try to continue sanely.  no-timeout ~= saturated timeout.
            n = (std::numeric_limits<int64_t>::max)();
        }

        // Kernel APIs validate timespecs as being at or after the epoch,
        // despite the kernel time type being signed.  However, no one can
        // tell the difference between a timeout at or before the epoch (since
        // all such timeouts have expired!)
        if (n < 0) n = 0;

        struct timespec abstime;
        int64_t seconds = (std::min)(n / kNanosPerSecond,
                                     int64_t{(std::numeric_limits<time_t>::max)()});
        abstime.tv_sec = static_cast<time_t>(seconds);
        abstime.tv_nsec =
                static_cast<decltype(abstime.tv_nsec)>(n % kNanosPerSecond);
        return abstime;
    }

  private:
    // internal rep, not user visible: ns after unix epoch.
    // zero = no timeout.
    // Negative we treat as an unlikely (and certainly expired!) but valid
    // timeout.
    int64_t _ns;

    static int64_t make_ns(abel::time_point t) {
        // optimization--infinite_future is common "no timeout" value
        // and cheaper to compare than convert.
        if (t == abel::time_point::infinite_future()) return 0;
        int64_t x = t.to_unix_nanos();

        // A timeout that lands exactly on the epoch (x=0) needs to be respected,
        // so we alter it unnoticably to 1.  Negative timeouts are in
        // theory supported, but handled poorly by the kernel (long
        // delays) so push them forward too; since all such times have
        // already passed, it's indistinguishable.
        if (x <= 0) x = 1;
        // A time larger than what can be represented to the kernel is treated
        // as no timeout.
        if (x == (std::numeric_limits<int64_t>::max)()) x = 0;
        return x;
    }

};

}  // namespace abel
#endif  // ABEL_CHRONO_TIMER_H_
