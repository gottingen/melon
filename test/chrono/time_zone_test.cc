//

#include <abel/chrono/internal/time_zone.h>

#include <gtest/gtest.h>
#include <testing/time_util.h>
#include <abel/chrono/time.h>

namespace {

TEST(time_zone, ValueSemantics) {
    abel::time_zone tz;
    abel::time_zone tz2 = tz;  // Copy-construct
    EXPECT_EQ(tz, tz2);
    tz2 = tz;  // Copy-assign
    EXPECT_EQ(tz, tz2);
}

TEST(time_zone, Equality) {
    abel::time_zone a, b;
    EXPECT_EQ(a, b);
    EXPECT_EQ(a.name(), b.name());

    abel::time_zone implicit_utc;
    abel::time_zone explicit_utc = abel::utc_time_zone();
    EXPECT_EQ(implicit_utc, explicit_utc);
    EXPECT_EQ(implicit_utc.name(), explicit_utc.name());

    abel::time_zone la = abel::chrono_internal::load_time_zone("America/Los_Angeles");
    abel::time_zone nyc = abel::chrono_internal::load_time_zone("America/New_York");
    EXPECT_NE(la, nyc);
}

TEST(time_zone, CCTZConversion) {
    const abel::chrono_internal::time_zone cz = abel::chrono_internal::utc_time_zone();
    const abel::time_zone tz(cz);
    EXPECT_EQ(cz, abel::chrono_internal::time_zone(tz));
}

TEST(time_zone, DefaultTimeZones) {
    abel::time_zone tz;
    EXPECT_EQ("UTC", abel::time_zone().name());
    EXPECT_EQ("UTC", abel::utc_time_zone().name());
}

TEST(time_zone, fixed_time_zone) {
    const abel::time_zone tz = abel::fixed_time_zone(123);
    const abel::chrono_internal::time_zone
        cz = abel::chrono_internal::fixed_time_zone(abel::chrono_internal::seconds(123));
    EXPECT_EQ(tz, abel::time_zone(cz));
}

TEST(time_zone, NamedTimeZones) {
    abel::time_zone nyc = abel::chrono_internal::load_time_zone("America/New_York");
    EXPECT_EQ("America/New_York", nyc.name());
    abel::time_zone syd = abel::chrono_internal::load_time_zone("Australia/Sydney");
    EXPECT_EQ("Australia/Sydney", syd.name());
    abel::time_zone fixed = abel::fixed_time_zone((((3 * 60) + 25) * 60) + 45);
    EXPECT_EQ("Fixed/UTC+03:25:45", fixed.name());
}

TEST(time_zone, Failures) {
    abel::time_zone tz(abel::chrono_internal::load_time_zone("America/Los_Angeles"));
    EXPECT_FALSE(load_time_zone("Invalid/time_zone", &tz));
    EXPECT_EQ(abel::utc_time_zone(), tz);  // guaranteed fallback to UTC

    // Ensures that the load still fails on a subsequent attempt.
    tz = abel::chrono_internal::load_time_zone("America/Los_Angeles");
    EXPECT_FALSE(load_time_zone("Invalid/time_zone", &tz));
    EXPECT_EQ(abel::utc_time_zone(), tz);  // guaranteed fallback to UTC

    // Loading an empty std::string timezone should fail.
    tz = abel::chrono_internal::load_time_zone("America/Los_Angeles");
    EXPECT_FALSE(load_time_zone("", &tz));
    EXPECT_EQ(abel::utc_time_zone(), tz);  // guaranteed fallback to UTC


}

/*
TEST(time_zone, local_time_zone) {
  const abel::time_zone local_tz = abel::local_time_zone();
  abel::time_zone tz = abel::chrono_internal::load_time_zone("localtime");
  EXPECT_EQ(tz, local_tz);
}
*/

}  // namespace
