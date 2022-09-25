
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#include "melon/times/internal/time_zone.h"

#include "testing/gtest_wrap.h"
#include "testing/time_util.h"
#include "melon/times/time.h"

namespace {

    TEST(time_zone, ValueSemantics) {
        melon::time_zone tz;
        melon::time_zone tz2 = tz;  // Copy-construct
        EXPECT_EQ(tz, tz2);
        tz2 = tz;  // Copy-assign
        EXPECT_EQ(tz, tz2);
    }

    TEST(time_zone, Equality) {
        melon::time_zone a, b;
        EXPECT_EQ(a, b);
        EXPECT_EQ(a.name(), b.name());

        melon::time_zone implicit_utc;
        melon::time_zone explicit_utc = melon::utc_time_zone();
        EXPECT_EQ(implicit_utc, explicit_utc);
        EXPECT_EQ(implicit_utc.name(), explicit_utc.name());

        melon::time_zone la = melon::times_internal::load_time_zone("America/Los_Angeles");
        melon::time_zone nyc = melon::times_internal::load_time_zone("America/New_York");
        EXPECT_NE(la, nyc);
    }

    TEST(time_zone, CCTZConversion) {
        const melon::times_internal::time_zone cz = melon::times_internal::utc_time_zone();
        const melon::time_zone tz(cz);
        EXPECT_EQ(cz, melon::times_internal::time_zone(tz));
    }

    TEST(time_zone, DefaultTimeZones) {
        melon::time_zone tz;
        EXPECT_EQ("UTC", melon::time_zone().name());
        EXPECT_EQ("UTC", melon::utc_time_zone().name());
    }

    TEST(time_zone, fixed_time_zone) {
        const melon::time_zone tz = melon::fixed_time_zone(123);
        const melon::times_internal::time_zone
                cz = melon::times_internal::fixed_time_zone(melon::times_internal::seconds(123));
        EXPECT_EQ(tz, melon::time_zone(cz));
    }

    TEST(time_zone, NamedTimeZones) {
        melon::time_zone nyc = melon::times_internal::load_time_zone("America/New_York");
        EXPECT_EQ("America/New_York", nyc.name());
        melon::time_zone syd = melon::times_internal::load_time_zone("Australia/Sydney");
        EXPECT_EQ("Australia/Sydney", syd.name());
        melon::time_zone fixed = melon::fixed_time_zone((((3 * 60) + 25) * 60) + 45);
        EXPECT_EQ("Fixed/UTC+03:25:45", fixed.name());
    }

    TEST(time_zone, Failures) {
        melon::time_zone tz(melon::times_internal::load_time_zone("America/Los_Angeles"));
        EXPECT_FALSE(load_time_zone("Invalid/time_zone", &tz));
        EXPECT_EQ(melon::utc_time_zone(), tz);  // guaranteed fallback to UTC

        // Ensures that the load still fails on a subsequent attempt.
        tz = melon::times_internal::load_time_zone("America/Los_Angeles");
        EXPECT_FALSE(load_time_zone("Invalid/time_zone", &tz));
        EXPECT_EQ(melon::utc_time_zone(), tz);  // guaranteed fallback to UTC

        // Loading an empty std::string timezone should fail.
        tz = melon::times_internal::load_time_zone("America/Los_Angeles");
        EXPECT_FALSE(load_time_zone("", &tz));
        EXPECT_EQ(melon::utc_time_zone(), tz);  // guaranteed fallback to UTC


    }

/*
TEST(time_zone, local_time_zone) {
  const melon::time_zone local_tz = melon::local_time_zone();
  melon::time_zone tz = melon::times_internal::load_time_zone("localtime");
  EXPECT_EQ(tz, local_tz);
}
*/

}  // namespace
