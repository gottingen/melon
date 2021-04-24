// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com

#include "abel/chrono/clock.h"
#include "abel/base/profile.h"

#if defined(ABEL_HAVE_ALARM)

#include <signal.h>
#include <unistd.h>

#elif defined(__linux__) || defined(__APPLE__)
#error all known Linux and Apple targets have alarm
#endif

#include "gtest/gtest.h"
#include "abel/chrono/time.h"

namespace {

    TEST(time_point, Now) {
        const abel::time_point before = abel::time_point::from_unix_nanos(abel::get_current_time_nanos());
        const abel::time_point now = abel::time_now();
        const abel::time_point after = abel::time_point::from_unix_nanos(abel::get_current_time_nanos());
        EXPECT_GE(now, before);
        EXPECT_GE(after, now);
    }

    enum class AlarmPolicy {
        kWithoutAlarm, kWithAlarm
    };

#if defined(ABEL_HAVE_ALARM)
    bool alarm_handler_invoked = false;

    void AlarmHandler(int signo) {
        ASSERT_EQ(signo, SIGALRM);
        alarm_handler_invoked = true;
    }

#endif

// Does sleep_for(d) take between lower_bound and upper_bound at least
// once between now and (now + timeout)?  If requested (and supported),
// add an alarm for the middle of the sleep period and expect it to fire.
    bool SleepForBounded(abel::duration d, abel::duration lower_bound,
                         abel::duration upper_bound, abel::duration timeout,
                         AlarmPolicy alarm_policy, int *attempts) {
        const abel::time_point deadline = abel::time_now() + timeout;
        while (abel::time_now() < deadline) {
#if defined(ABEL_HAVE_ALARM)
            sig_t old_alarm = SIG_DFL;
            if (alarm_policy == AlarmPolicy::kWithAlarm) {
                alarm_handler_invoked = false;
                old_alarm = signal(SIGALRM, AlarmHandler);
                alarm((d / 2).to_int64_seconds());
            }
#else
            EXPECT_EQ(alarm_policy, AlarmPolicy::kWithoutAlarm);
#endif
            ++*attempts;
            abel::time_point start = abel::time_now();
            abel::sleep_for(d);
            abel::duration actual = abel::time_now() - start;
#if defined(ABEL_HAVE_ALARM)
            if (alarm_policy == AlarmPolicy::kWithAlarm) {
                signal(SIGALRM, old_alarm);
                if (!alarm_handler_invoked)
                    continue;
            }
#endif
            if (lower_bound <= actual && actual <= upper_bound) {
                return true;  // yes, the sleep_for() was correctly bounded
            }
        }
        return false;
    }

    testing::AssertionResult AssertSleepForBounded(abel::duration d,
                                                   abel::duration early,
                                                   abel::duration late,
                                                   abel::duration timeout,
                                                   AlarmPolicy alarm_policy) {
        const abel::duration lower_bound = d - early;
        const abel::duration upper_bound = d + late;
        int attempts = 0;
        if (SleepForBounded(d, lower_bound, upper_bound, timeout, alarm_policy,
                            &attempts)) {
            return testing::AssertionSuccess();
        }
        return testing::AssertionFailure()
                << "sleep_for(" << d << ") did not return within [" << lower_bound
                << ":" << upper_bound << "] in " << attempts << " attempt"
                << (attempts == 1 ? "" : "s") << " over " << timeout
                << (alarm_policy == AlarmPolicy::kWithAlarm ? " with" : " without")
                << " an alarm";
    }

// Tests that sleep_for() returns neither too early nor too late.
    TEST(sleep_for, Bounded) {
        const abel::duration d = abel::duration::milliseconds(2500);
        const abel::duration early = abel::duration::milliseconds(100);
        const abel::duration late = abel::duration::milliseconds(300);
        const abel::duration timeout = 48 * d;
        EXPECT_TRUE(AssertSleepForBounded(d, early, late, timeout,
                                          AlarmPolicy::kWithoutAlarm));
#if defined(ABEL_HAVE_ALARM)
        EXPECT_TRUE(AssertSleepForBounded(d, early, late, timeout,
                                          AlarmPolicy::kWithAlarm));
#endif
    }

}  // namespace
