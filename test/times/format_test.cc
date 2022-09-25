
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#include <cstdint>
#include <limits>
#include <string>


#include "testing/gtest_wrap.h"
#include "testing/time_util.h"
#include "melon/times/time.h"

using testing::HasSubstr;

namespace {

// A helper that tests the given format specifier by itself, and with leading
// and trailing characters.  For example: TestFormatSpecifier(t, "%a", "Thu").
    void TestFormatSpecifier(melon::time_point t, melon::time_zone tz,
                             const std::string &fmt, const std::string &ans) {
        EXPECT_EQ(ans, melon::format_time(fmt, t, tz));
        EXPECT_EQ("xxx " + ans, melon::format_time("xxx " + fmt, t, tz));
        EXPECT_EQ(ans + " yyy", melon::format_time(fmt + " yyy", t, tz));
        EXPECT_EQ("xxx " + ans + " yyy",
                  melon::format_time("xxx " + fmt + " yyy", t, tz));
    }

//
// Testing format_time()
//

    TEST(format_time, Basics) {
        melon::time_zone tz = melon::utc_time_zone();
        melon::time_point t = melon::time_point::from_time_t(0);

        // Starts with a couple basic edge cases.
        EXPECT_EQ("", melon::format_time("", t, tz));
        EXPECT_EQ(" ", melon::format_time(" ", t, tz));
        EXPECT_EQ("  ", melon::format_time("  ", t, tz));
        EXPECT_EQ("xxx", melon::format_time("xxx", t, tz));
        std::string big(128, 'x');
        EXPECT_EQ(big, melon::format_time(big, t, tz));
        // Cause the 1024-byte buffer to grow.
        std::string bigger(100000, 'x');
        EXPECT_EQ(bigger, melon::format_time(bigger, t, tz));

        t += melon::duration::hours(13) + melon::duration::minutes(4) + melon::duration::seconds(5);
        t += melon::duration::milliseconds(6) + melon::duration::microseconds(7) + melon::duration::nanoseconds(8);
        EXPECT_EQ("1970-01-01", melon::format_time("%Y-%m-%d", t, tz));
        EXPECT_EQ("13:04:05", melon::format_time("%H:%M:%S", t, tz));
        EXPECT_EQ("13:04:05.006", melon::format_time("%H:%M:%E3S", t, tz));
        EXPECT_EQ("13:04:05.006007", melon::format_time("%H:%M:%E6S", t, tz));
        EXPECT_EQ("13:04:05.006007008", melon::format_time("%H:%M:%E9S", t, tz));
    }

    TEST(format_time, LocaleSpecific) {
        const melon::time_zone tz = melon::utc_time_zone();
        melon::time_point t = melon::time_point::from_time_t(0);

        TestFormatSpecifier(t, tz, "%a", "Thu");
        TestFormatSpecifier(t, tz, "%A", "Thursday");
        TestFormatSpecifier(t, tz, "%b", "Jan");
        TestFormatSpecifier(t, tz, "%B", "January");

        // %c should at least produce the numeric year and time-of-day.
        const std::string s =
                melon::format_time("%c", melon::time_point::from_time_t(0), melon::utc_time_zone());
        EXPECT_THAT(s, HasSubstr("1970"));
        EXPECT_THAT(s, HasSubstr("00:00:00"));

        TestFormatSpecifier(t, tz, "%p", "AM");
        TestFormatSpecifier(t, tz, "%x", "01/01/70");
        TestFormatSpecifier(t, tz, "%X", "00:00:00");
    }

    TEST(format_time, ExtendedSeconds) {
        const melon::time_zone tz = melon::utc_time_zone();

        // No subseconds.
        melon::time_point t = melon::time_point::from_time_t(0) + melon::duration::seconds(5);
        EXPECT_EQ("05", melon::format_time("%E*S", t, tz));
        EXPECT_EQ("05.000000000000000", melon::format_time("%E15S", t, tz));

        // With subseconds.
        t += melon::duration::milliseconds(6) + melon::duration::microseconds(7) + melon::duration::nanoseconds(8);
        EXPECT_EQ("05.006007008", melon::format_time("%E*S", t, tz));
        EXPECT_EQ("05", melon::format_time("%E0S", t, tz));
        EXPECT_EQ("05.006007008000000", melon::format_time("%E15S", t, tz));

        // Times before the Unix epoch.
        t = melon::time_point::from_unix_micros(-1);
        EXPECT_EQ("1969-12-31 23:59:59.999999",
                  melon::format_time("%Y-%m-%d %H:%M:%E*S", t, tz));

        // Here is a "%E*S" case we got wrong for a while.  While the first
        // instant below is correctly rendered as "...:07.333304", the second
        // one used to appear as "...:07.33330499999999999".
        t = melon::time_point::from_unix_micros(1395024427333304);
        EXPECT_EQ("2014-03-17 02:47:07.333304",
                  melon::format_time("%Y-%m-%d %H:%M:%E*S", t, tz));
        t += melon::duration::microseconds(1);
        EXPECT_EQ("2014-03-17 02:47:07.333305",
                  melon::format_time("%Y-%m-%d %H:%M:%E*S", t, tz));
    }

    TEST(format_time, RFC1123FormatPadsYear) {  // locale specific
        melon::time_zone tz = melon::utc_time_zone();

        // A year of 77 should be padded to 0077.
        melon::time_point t = melon::from_chrono(melon::chrono_second(77, 6, 28, 9, 8, 7), tz);
        EXPECT_EQ("Mon, 28 Jun 0077 09:08:07 +0000",
                  melon::format_time(melon::RFC1123_full, t, tz));
        EXPECT_EQ("28 Jun 0077 09:08:07 +0000",
                  melon::format_time(melon::RFC1123_no_wday, t, tz));
    }

    TEST(format_time, InfiniteTime) {
        melon::time_zone tz = melon::times_internal::load_time_zone("America/Los_Angeles");

        // The format and timezone are ignored.
        EXPECT_EQ("infinite-future",
                  melon::format_time("%H:%M blah", melon::time_point::infinite_future(), tz));
        EXPECT_EQ("infinite-past",
                  melon::format_time("%H:%M blah", melon::time_point::infinite_past(), tz));
    }

//
// Testing parse_time()
//

    TEST(parse_time, Basics) {
        melon::time_point t = melon::time_point::from_time_t(1234567890);
        std::string err;

        // Simple edge cases.
        EXPECT_TRUE(melon::parse_time("", "", &t, &err)) << err;
        EXPECT_EQ(melon::time_point::unix_epoch(), t);  // everything defaulted
        EXPECT_TRUE(melon::parse_time(" ", " ", &t, &err)) << err;
        EXPECT_TRUE(melon::parse_time("  ", "  ", &t, &err)) << err;
        EXPECT_TRUE(melon::parse_time("x", "x", &t, &err)) << err;
        EXPECT_TRUE(melon::parse_time("xxx", "xxx", &t, &err)) << err;

        EXPECT_TRUE(melon::parse_time("%Y-%m-%d %H:%M:%S %z",
                                     "2013-06-28 19:08:09 -0800", &t, &err))
                            << err;
        const auto ci = melon::fixed_time_zone(-8 * 60 * 60).at(t);
        EXPECT_EQ(melon::chrono_second(2013, 6, 28, 19, 8, 9), ci.cs);
        EXPECT_EQ(melon::zero_duration(), ci.subsecond);
    }

    TEST(parse_time, NullErrorString) {
        melon::time_point t;
        EXPECT_FALSE(melon::parse_time("%Q", "invalid format", &t, nullptr));
        EXPECT_FALSE(melon::parse_time("%H", "12 trailing data", &t, nullptr));
        EXPECT_FALSE(
                melon::parse_time("%H out of range", "42 out of range", &t, nullptr));
    }

    TEST(parse_time, WithTimeZone) {
        const melon::time_zone tz =
                melon::times_internal::load_time_zone("America/Los_Angeles");
        melon::time_point t;
        std::string e;

        // We can parse a std::string without a UTC offset if we supply a timezone.
        EXPECT_TRUE(
                melon::parse_time("%Y-%m-%d %H:%M:%S", "2013-06-28 19:08:09", tz, &t, &e))
                            << e;
        auto ci = tz.at(t);
        EXPECT_EQ(melon::chrono_second(2013, 6, 28, 19, 8, 9), ci.cs);
        EXPECT_EQ(melon::zero_duration(), ci.subsecond);

        // But the timezone is ignored when a UTC offset is present.
        EXPECT_TRUE(melon::parse_time("%Y-%m-%d %H:%M:%S %z",
                                     "2013-06-28 19:08:09 +0800", tz, &t, &e))
                            << e;
        ci = melon::fixed_time_zone(8 * 60 * 60).at(t);
        EXPECT_EQ(melon::chrono_second(2013, 6, 28, 19, 8, 9), ci.cs);
        EXPECT_EQ(melon::zero_duration(), ci.subsecond);
    }

    TEST(parse_time, ErrorCases) {
        melon::time_point t = melon::time_point::from_time_t(0);
        std::string err;

        EXPECT_FALSE(melon::parse_time("%S", "123", &t, &err)) << err;
        EXPECT_THAT(err, HasSubstr("Illegal trailing data"));

        // Can't parse an illegal format specifier.
        err.clear();
        EXPECT_FALSE(melon::parse_time("%Q", "x", &t, &err)) << err;
        // Exact contents of "err" are platform-dependent because of
        // differences in the strptime implementation between macOS and Linux.
        EXPECT_FALSE(err.empty());

        // Fails because of trailing, unparsed data "blah".
        EXPECT_FALSE(melon::parse_time("%m-%d", "2-3 blah", &t, &err)) << err;
        EXPECT_THAT(err, HasSubstr("Illegal trailing data"));

        // Feb 31 requires normalization.
        EXPECT_FALSE(melon::parse_time("%m-%d", "2-31", &t, &err)) << err;
        EXPECT_THAT(err, HasSubstr("Out-of-range"));

        // Check that we cannot have spaces in UTC offsets.
        EXPECT_TRUE(melon::parse_time("%z", "-0203", &t, &err)) << err;
        EXPECT_FALSE(melon::parse_time("%z", "- 2 3", &t, &err)) << err;
        EXPECT_THAT(err, HasSubstr("Failed to parse"));
        EXPECT_TRUE(melon::parse_time("%Ez", "-02:03", &t, &err)) << err;
        EXPECT_FALSE(melon::parse_time("%Ez", "- 2: 3", &t, &err)) << err;
        EXPECT_THAT(err, HasSubstr("Failed to parse"));

        // Check that we reject other malformed UTC offsets.
        EXPECT_FALSE(melon::parse_time("%Ez", "+-08:00", &t, &err)) << err;
        EXPECT_THAT(err, HasSubstr("Failed to parse"));
        EXPECT_FALSE(melon::parse_time("%Ez", "-+08:00", &t, &err)) << err;
        EXPECT_THAT(err, HasSubstr("Failed to parse"));

        // Check that we do not accept "-0" in fields that allow zero.
        EXPECT_FALSE(melon::parse_time("%Y", "-0", &t, &err)) << err;
        EXPECT_THAT(err, HasSubstr("Failed to parse"));
        EXPECT_FALSE(melon::parse_time("%E4Y", "-0", &t, &err)) << err;
        EXPECT_THAT(err, HasSubstr("Failed to parse"));
        EXPECT_FALSE(melon::parse_time("%H", "-0", &t, &err)) << err;
        EXPECT_THAT(err, HasSubstr("Failed to parse"));
        EXPECT_FALSE(melon::parse_time("%M", "-0", &t, &err)) << err;
        EXPECT_THAT(err, HasSubstr("Failed to parse"));
        EXPECT_FALSE(melon::parse_time("%S", "-0", &t, &err)) << err;
        EXPECT_THAT(err, HasSubstr("Failed to parse"));
        EXPECT_FALSE(melon::parse_time("%z", "+-000", &t, &err)) << err;
        EXPECT_THAT(err, HasSubstr("Failed to parse"));
        EXPECT_FALSE(melon::parse_time("%Ez", "+-0:00", &t, &err)) << err;
        EXPECT_THAT(err, HasSubstr("Failed to parse"));
        EXPECT_FALSE(melon::parse_time("%z", "-00-0", &t, &err)) << err;
        EXPECT_THAT(err, HasSubstr("Illegal trailing data"));
        EXPECT_FALSE(melon::parse_time("%Ez", "-00:-0", &t, &err)) << err;
        EXPECT_THAT(err, HasSubstr("Illegal trailing data"));
    }

    TEST(parse_time, ExtendedSeconds) {
        std::string err;
        melon::time_point t;

        // Here is a "%E*S" case we got wrong for a while.  The fractional
        // part of the first instant is less than 2^31 and was correctly
        // parsed, while the second (and any subsecond field >=2^31) failed.
        t = melon::time_point::unix_epoch();
        EXPECT_TRUE(melon::parse_time("%E*S", "0.2147483647", &t, &err)) << err;
        EXPECT_EQ(melon::time_point::unix_epoch() + melon::duration::nanoseconds(214748364) +
                  melon::duration::nanoseconds(1) / 2,
                  t);
        t = melon::time_point::unix_epoch();
        EXPECT_TRUE(melon::parse_time("%E*S", "0.2147483648", &t, &err)) << err;
        EXPECT_EQ(melon::time_point::unix_epoch() + melon::duration::nanoseconds(214748364) +
                  melon::duration::nanoseconds(3) / 4,
                  t);

        // We should also be able to specify long strings of digits far
        // beyond the current resolution and have them convert the same way.
        t = melon::time_point::unix_epoch();
        EXPECT_TRUE(melon::parse_time(
                "%E*S", "0.214748364801234567890123456789012345678901234567890123456789",
                &t, &err))
                            << err;
        EXPECT_EQ(melon::time_point::unix_epoch() + melon::duration::nanoseconds(214748364) +
                  melon::duration::nanoseconds(3) / 4,
                  t);
    }

    TEST(parse_time, ExtendedOffsetErrors) {
        std::string err;
        melon::time_point t;

        // %z against +-HHMM.
        EXPECT_FALSE(melon::parse_time("%z", "-123", &t, &err)) << err;
        EXPECT_THAT(err, HasSubstr("Illegal trailing data"));

        // %z against +-HH.
        EXPECT_FALSE(melon::parse_time("%z", "-1", &t, &err)) << err;
        EXPECT_THAT(err, HasSubstr("Failed to parse"));

        // %Ez against +-HH:MM.
        EXPECT_FALSE(melon::parse_time("%Ez", "-12:3", &t, &err)) << err;
        EXPECT_THAT(err, HasSubstr("Illegal trailing data"));

        // %Ez against +-HHMM.
        EXPECT_FALSE(melon::parse_time("%Ez", "-123", &t, &err)) << err;
        EXPECT_THAT(err, HasSubstr("Illegal trailing data"));

        // %Ez against +-HH.
        EXPECT_FALSE(melon::parse_time("%Ez", "-1", &t, &err)) << err;
        EXPECT_THAT(err, HasSubstr("Failed to parse"));
    }

    TEST(parse_time, InfiniteTime) {
        melon::time_point t;
        std::string err;
        EXPECT_TRUE(melon::parse_time("%H:%M blah", "infinite-future", &t, &err));
        EXPECT_EQ(melon::time_point::infinite_future(), t);

        // Surrounding whitespace.
        EXPECT_TRUE(melon::parse_time("%H:%M blah", "  infinite-future", &t, &err));
        EXPECT_EQ(melon::time_point::infinite_future(), t);
        EXPECT_TRUE(melon::parse_time("%H:%M blah", "infinite-future  ", &t, &err));
        EXPECT_EQ(melon::time_point::infinite_future(), t);
        EXPECT_TRUE(melon::parse_time("%H:%M blah", "  infinite-future  ", &t, &err));
        EXPECT_EQ(melon::time_point::infinite_future(), t);

        EXPECT_TRUE(melon::parse_time("%H:%M blah", "infinite-past", &t, &err));
        EXPECT_EQ(melon::time_point::infinite_past(), t);

        // Surrounding whitespace.
        EXPECT_TRUE(melon::parse_time("%H:%M blah", "  infinite-past", &t, &err));
        EXPECT_EQ(melon::time_point::infinite_past(), t);
        EXPECT_TRUE(melon::parse_time("%H:%M blah", "infinite-past  ", &t, &err));
        EXPECT_EQ(melon::time_point::infinite_past(), t);
        EXPECT_TRUE(melon::parse_time("%H:%M blah", "  infinite-past  ", &t, &err));
        EXPECT_EQ(melon::time_point::infinite_past(), t);

        // "infinite-future" as literal std::string
        melon::time_zone tz = melon::utc_time_zone();
        EXPECT_TRUE(melon::parse_time("infinite-future %H:%M", "infinite-future 03:04",
                                     &t, &err));
        EXPECT_NE(melon::time_point::infinite_future(), t);
        EXPECT_EQ(3, tz.at(t).cs.hour());
        EXPECT_EQ(4, tz.at(t).cs.minute());

        // "infinite-past" as literal std::string
        EXPECT_TRUE(
                melon::parse_time("infinite-past %H:%M", "infinite-past 03:04", &t, &err));
        EXPECT_NE(melon::time_point::infinite_past(), t);
        EXPECT_EQ(3, tz.at(t).cs.hour());
        EXPECT_EQ(4, tz.at(t).cs.minute());

        // The input doesn't match the format.
        EXPECT_FALSE(melon::parse_time("infinite-future %H:%M", "03:04", &t, &err));
        EXPECT_FALSE(melon::parse_time("infinite-past %H:%M", "03:04", &t, &err));
    }

    TEST(parse_time, FailsOnUnrepresentableTime) {
        const melon::time_zone utc = melon::utc_time_zone();
        melon::time_point t;
        EXPECT_FALSE(
                melon::parse_time("%Y-%m-%d", "-292277022657-01-27", utc, &t, nullptr));
        EXPECT_TRUE(
                melon::parse_time("%Y-%m-%d", "-292277022657-01-28", utc, &t, nullptr));
        EXPECT_TRUE(
                melon::parse_time("%Y-%m-%d", "292277026596-12-04", utc, &t, nullptr));
        EXPECT_FALSE(
                melon::parse_time("%Y-%m-%d", "292277026596-12-05", utc, &t, nullptr));
    }

//
// Roundtrip test for format_time()/parse_time().
//

    TEST(FormatParse, RoundTrip) {
        const melon::time_zone lax =
                melon::times_internal::load_time_zone("America/Los_Angeles");
        const melon::time_point in =
                melon::from_chrono(melon::chrono_second(1977, 6, 28, 9, 8, 7), lax);
        const melon::duration subseconds = melon::duration::nanoseconds(654321);
        std::string err;

        // RFC3339, which renders subseconds.
        {
            melon::time_point out;
            const std::string s =
                    melon::format_time(melon::RFC3339_full, in + subseconds, lax);
            EXPECT_TRUE(melon::parse_time(melon::RFC3339_full, s, &out, &err))
                                << s << ": " << err;
            EXPECT_EQ(in + subseconds, out);  // RFC3339_full includes %Ez
        }

        // RFC1123, which only does whole seconds.
        {
            melon::time_point out;
            const std::string s = melon::format_time(melon::RFC1123_full, in, lax);
            EXPECT_TRUE(melon::parse_time(melon::RFC1123_full, s, &out, &err))
                                << s << ": " << err;
            EXPECT_EQ(in, out);  // RFC1123_full includes %z
        }

        // `melon::format_time()` falls back to strftime() for "%c", which appears to
        // work. On Windows, `melon::parse_time()` falls back to std::get_time() which
        // appears to fail on "%c" (or at least on the "%c" text produced by
        // `strftime()`). This makes it fail the round-trip test.
        //
        // Under the emscripten compiler `melon::parse_time() falls back to
        // `strptime()`, but that ends up using a different definition for "%c"
        // compared to `strftime()`, also causing the round-trip test to fail
        // (see https://github.com/kripken/emscripten/pull/7491).
#if !defined(_MSC_VER) && !defined(__EMSCRIPTEN__)
        // Even though we don't know what %c will produce, it should roundtrip,
        // but only in the 0-offset timezone.
        {
            melon::time_point out;
            const std::string s = melon::format_time("%c", in, melon::utc_time_zone());
            EXPECT_TRUE(melon::parse_time("%c", s, &out, &err)) << s << ": " << err;
            EXPECT_EQ(in, out);
        }
#endif  // !_MSC_VER && !__EMSCRIPTEN__
    }

    TEST(FormatParse, RoundTripDistantFuture) {
        const melon::time_zone tz = melon::utc_time_zone();
        const melon::time_point in =
                melon::time_point::from_unix_seconds(std::numeric_limits<int64_t>::max());
        std::string err;

        melon::time_point out;
        const std::string s = melon::format_time(melon::RFC3339_full, in, tz);
        EXPECT_TRUE(melon::parse_time(melon::RFC3339_full, s, &out, &err))
                            << s << ": " << err;
        EXPECT_EQ(in, out);
    }

    TEST(FormatParse, RoundTripDistantPast) {
        const melon::time_zone tz = melon::utc_time_zone();
        const melon::time_point in =
                melon::time_point::from_unix_seconds(std::numeric_limits<int64_t>::min());
        std::string err;

        melon::time_point out;
        const std::string s = melon::format_time(melon::RFC3339_full, in, tz);
        EXPECT_TRUE(melon::parse_time(melon::RFC3339_full, s, &out, &err))
                            << s << ": " << err;
        EXPECT_EQ(in, out);
    }

}  // namespace
