
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/
#include "melon/times/time.h"
#include "melon/base/profile.h"
#include <signal.h>
#include <unistd.h>
#include "testing/gtest_wrap.h"

namespace {

    TEST(time_point, Now) {
        const melon::time_point before = melon::time_point::from_unix_nanos(melon::get_current_time_nanos());
        const melon::time_point now = melon::time_now();
        const melon::time_point after = melon::time_point::from_unix_nanos(melon::get_current_time_nanos());
        EXPECT_GE(now, before);
        EXPECT_GE(after, now);
    }

    enum class AlarmPolicy {
        kWithoutAlarm, kWithAlarm
    };

    bool alarm_handler_invoked = false;

    void AlarmHandler(int signo) {
        ASSERT_EQ(signo, SIGALRM);
        alarm_handler_invoked = true;
    }


    // Does sleep_for(d) take between lower_bound and upper_bound at least
    // once between now and (now + timeout)?  If requested (and supported),
    // add an alarm for the middle of the sleep period and expect it to fire.
    bool SleepForBounded(melon::duration d, melon::duration lower_bound,
                         melon::duration upper_bound, melon::duration timeout,
                         AlarmPolicy alarm_policy, int *attempts) {
        const melon::time_point deadline = melon::time_now() + timeout;
        while (melon::time_now() < deadline) {
            sig_t old_alarm = SIG_DFL;
            if (alarm_policy == AlarmPolicy::kWithAlarm) {
                alarm_handler_invoked = false;
                old_alarm = signal(SIGALRM, AlarmHandler);
                alarm((d / 2).to_int64_seconds());
            }
            ++*attempts;
            melon::time_point start = melon::time_now();
            melon::sleep_for(d);
            melon::duration actual = melon::time_now() - start;
            if (alarm_policy == AlarmPolicy::kWithAlarm) {
                signal(SIGALRM, old_alarm);
                if (!alarm_handler_invoked)
                    continue;
            }
            if (lower_bound <= actual && actual <= upper_bound) {
                return true;  // yes, the sleep_for() was correctly bounded
            }
        }
        return false;
    }

    testing::AssertionResult AssertSleepForBounded(melon::duration d,
                                                   melon::duration early,
                                                   melon::duration late,
                                                   melon::duration timeout,
                                                   AlarmPolicy alarm_policy) {
        const melon::duration lower_bound = d - early;
        const melon::duration upper_bound = d + late;
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
        const melon::duration d = melon::duration::milliseconds(2500);
        const melon::duration early = melon::duration::milliseconds(100);
        const melon::duration late = melon::duration::milliseconds(300);
        const melon::duration timeout = 48 * d;
        EXPECT_TRUE(AssertSleepForBounded(d, early, late, timeout,
                                          AlarmPolicy::kWithoutAlarm));
        EXPECT_TRUE(AssertSleepForBounded(d, early, late, timeout,
                                          AlarmPolicy::kWithAlarm));
    }

}  // namespace
