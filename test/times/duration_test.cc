
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#if defined(_MSC_VER)
#include <winsock2.h>  // for timeval
#endif

#include <chrono>  // NOLINT(build/c++11)
#include <cmath>
#include <cstdint>
#include <ctime>
#include <iomanip>
#include <limits>
#include <random>
#include <string>


#include "testing/gtest_wrap.h"
#include "melon/times/time.h"

namespace {

    constexpr int64_t kint64max = std::numeric_limits<int64_t>::max();
    constexpr int64_t kint64min = std::numeric_limits<int64_t>::min();

// Approximates the given number of years. This is only used to make some test
// code more readable.
    melon::duration ApproxYears(int64_t n) { return melon::duration::hours(n) * 365 * 24; }

// A gMock matcher to match timespec values. Use this matcher like:
// timespec ts1, ts2;
// EXPECT_THAT(ts1, TimespecMatcher(ts2));
    MATCHER_P(TimespecMatcher, ts, "") {
        if (ts.tv_sec == arg.tv_sec && ts.tv_nsec == arg.tv_nsec)
            return true;
        *result_listener << "expected: {" << ts.tv_sec << ", " << ts.tv_nsec << "} ";
        *result_listener << "actual: {" << arg.tv_sec << ", " << arg.tv_nsec << "}";
        return false;
    }

// A gMock matcher to match timeval values. Use this matcher like:
// timeval tv1, tv2;
// EXPECT_THAT(tv1, TimevalMatcher(tv2));
    MATCHER_P(TimevalMatcher, tv, "") {
        if (tv.tv_sec == arg.tv_sec && tv.tv_usec == arg.tv_usec)
            return true;
        *result_listener << "expected: {" << tv.tv_sec << ", " << tv.tv_usec << "} ";
        *result_listener << "actual: {" << arg.tv_sec << ", " << arg.tv_usec << "}";
        return false;
    }

    TEST(duration, ConstExpr) {
        constexpr melon::duration d0 = melon::zero_duration();
        static_assert(d0 == melon::zero_duration(), "zero_duration()");
        constexpr melon::duration d1 = melon::duration::seconds(1);
        static_assert(d1 == melon::duration::seconds(1), "seconds(1)");
        static_assert(d1 != melon::zero_duration(), "seconds(1)");
        constexpr melon::duration d2 = melon::infinite_duration();
        static_assert(d2 == melon::infinite_duration(), "infinite_duration()");
        static_assert(d2 != melon::zero_duration(), "infinite_duration()");
    }

    TEST(duration, ValueSemantics) {
        // If this compiles, the test passes.
        constexpr melon::duration a;      // Default construction
        constexpr melon::duration b = a;  // Copy construction
        constexpr melon::duration c(b);   // Copy construction (again)

        melon::duration d;
        d = c;  // Assignment
    }

    TEST(duration, Factories) {
        constexpr melon::duration zero = melon::zero_duration();
        constexpr melon::duration nano = melon::duration::nanoseconds(1);
        constexpr melon::duration micro = melon::duration::microseconds(1);
        constexpr melon::duration milli = melon::duration::milliseconds(1);
        constexpr melon::duration sec = melon::duration::seconds(1);
        constexpr melon::duration min = melon::duration::minutes(1);
        constexpr melon::duration hour = melon::duration::hours(1);

        EXPECT_EQ(zero, melon::duration());
        EXPECT_EQ(zero, melon::duration::seconds(0));
        EXPECT_EQ(nano, melon::duration::nanoseconds(1));
        EXPECT_EQ(micro, melon::duration::nanoseconds(1000));
        EXPECT_EQ(milli, melon::duration::microseconds(1000));
        EXPECT_EQ(sec, melon::duration::milliseconds(1000));
        EXPECT_EQ(min, melon::duration::seconds(60));
        EXPECT_EQ(hour, melon::duration::minutes(60));

        // Tests factory limits
        const melon::duration inf = melon::infinite_duration();

        EXPECT_GT(inf, melon::duration::seconds(kint64max));
        EXPECT_LT(-inf, melon::duration::seconds(kint64min));
        EXPECT_LT(-inf, melon::duration::seconds(-kint64max));

        EXPECT_EQ(inf, melon::duration::minutes(kint64max));
        EXPECT_EQ(-inf, melon::duration::minutes(kint64min));
        EXPECT_EQ(-inf, melon::duration::minutes(-kint64max));
        EXPECT_GT(inf, melon::duration::minutes(kint64max / 60));
        EXPECT_LT(-inf, melon::duration::minutes(kint64min / 60));
        EXPECT_LT(-inf, melon::duration::minutes(-kint64max / 60));

        EXPECT_EQ(inf, melon::duration::hours(kint64max));
        EXPECT_EQ(-inf, melon::duration::hours(kint64min));
        EXPECT_EQ(-inf, melon::duration::hours(-kint64max));
        EXPECT_GT(inf, melon::duration::hours(kint64max / 3600));
        EXPECT_LT(-inf, melon::duration::hours(kint64min / 3600));
        EXPECT_LT(-inf, melon::duration::hours(-kint64max / 3600));
    }

    TEST(duration, ToConversion) {
#define TEST_DURATION_CONVERSION(UNIT)                                  \
  do {                                                                  \
    const melon::duration d = melon::duration::UNIT(1.5);                           \
    const melon::duration nd = -d;                                      \
    constexpr melon::duration z = melon::zero_duration();                  \
    constexpr melon::duration inf = melon::infinite_duration();            \
    constexpr melon::duration ninf = -inf;                                \
    constexpr double dbl_inf = std::numeric_limits<double>::infinity(); \
    EXPECT_EQ(kint64min, ninf.to_int64_##UNIT());                    \
    EXPECT_EQ(-1, nd.to_int64_##UNIT());                             \
    EXPECT_EQ(0, z.to_int64_##UNIT());                               \
    EXPECT_EQ(1, d.to_int64_##UNIT());                               \
    EXPECT_EQ(kint64max, inf.to_int64_##UNIT());                     \
    EXPECT_EQ(-dbl_inf, ninf.to_double_##UNIT());                    \
    EXPECT_EQ(-1.5, nd.to_double_##UNIT());                          \
    EXPECT_EQ(0, z.to_double_##UNIT());                              \
    EXPECT_EQ(1.5, d.to_double_##UNIT());                            \
    EXPECT_EQ(dbl_inf, inf.to_double_##UNIT());                      \
  } while (0)

        TEST_DURATION_CONVERSION(nanoseconds);
        TEST_DURATION_CONVERSION(microseconds);
        TEST_DURATION_CONVERSION(milliseconds);
        TEST_DURATION_CONVERSION(seconds);
        TEST_DURATION_CONVERSION(minutes);
        TEST_DURATION_CONVERSION(hours);

#undef TEST_DURATION_CONVERSION
    }

    template<int64_t N>
    void TestToConversion() {
        constexpr melon::duration nano = melon::duration::nanoseconds(N);
        EXPECT_EQ(N, nano.to_int64_nanoseconds());
        EXPECT_EQ(0, nano.to_int64_microseconds());
        EXPECT_EQ(0, nano.to_int64_milliseconds());
        EXPECT_EQ(0, nano.to_int64_seconds());
        EXPECT_EQ(0, nano.to_int64_minutes());
        EXPECT_EQ(0, nano.to_int64_hours());
        const melon::duration micro = melon::duration::microseconds(N);
        EXPECT_EQ(N * 1000, micro.to_int64_nanoseconds());
        EXPECT_EQ(N, micro.to_int64_microseconds());
        EXPECT_EQ(0, micro.to_int64_milliseconds());
        EXPECT_EQ(0, micro.to_int64_seconds());
        EXPECT_EQ(0, micro.to_int64_minutes());
        EXPECT_EQ(0, micro.to_int64_hours());
        const melon::duration milli = melon::duration::milliseconds(N);
        EXPECT_EQ(N * 1000 * 1000, milli.to_int64_nanoseconds());
        EXPECT_EQ(N * 1000, milli.to_int64_microseconds());
        EXPECT_EQ(N, milli.to_int64_milliseconds());
        EXPECT_EQ(0, milli.to_int64_seconds());
        EXPECT_EQ(0, milli.to_int64_minutes());
        EXPECT_EQ(0, milli.to_int64_hours());
        const melon::duration sec = melon::duration::seconds(N);
        EXPECT_EQ(N * 1000 * 1000 * 1000, sec.to_int64_nanoseconds());
        EXPECT_EQ(N * 1000 * 1000, sec.to_int64_microseconds());
        EXPECT_EQ(N * 1000, sec.to_int64_milliseconds());
        EXPECT_EQ(N, sec.to_int64_seconds());
        EXPECT_EQ(0, sec.to_int64_minutes());
        EXPECT_EQ(0, sec.to_int64_hours());
        const melon::duration min = melon::duration::minutes(N);
        EXPECT_EQ(N * 60 * 1000 * 1000 * 1000, min.to_int64_nanoseconds());
        EXPECT_EQ(N * 60 * 1000 * 1000, min.to_int64_microseconds());
        EXPECT_EQ(N * 60 * 1000, min.to_int64_milliseconds());
        EXPECT_EQ(N * 60, min.to_int64_seconds());
        EXPECT_EQ(N, min.to_int64_minutes());
        EXPECT_EQ(0, min.to_int64_hours());
        const melon::duration hour = melon::duration::hours(N);
        EXPECT_EQ(N * 60 * 60 * 1000 * 1000 * 1000, hour.to_int64_nanoseconds());
        EXPECT_EQ(N * 60 * 60 * 1000 * 1000, hour.to_int64_microseconds());
        EXPECT_EQ(N * 60 * 60 * 1000, hour.to_int64_milliseconds());
        EXPECT_EQ(N * 60 * 60, hour.to_int64_seconds());
        EXPECT_EQ(N * 60, hour.to_int64_minutes());
        EXPECT_EQ(N, hour.to_int64_hours());
    }

    TEST(duration, ToConversionDeprecated) {
        TestToConversion<43>();
        TestToConversion<1>();
        TestToConversion<0>();
        TestToConversion<-1>();
        TestToConversion<-43>();
    }

    template<int64_t N>
    void Testfrom_chronoBasicEquality() {
        using std::chrono::nanoseconds;
        using std::chrono::microseconds;
        using std::chrono::milliseconds;
        using std::chrono::seconds;
        using std::chrono::minutes;
        using std::chrono::hours;

        static_assert(melon::duration::nanoseconds(N) == melon::duration::from_chrono(nanoseconds(N)), "");
        static_assert(melon::duration::microseconds(N) == melon::duration::from_chrono(microseconds(N)), "");
        static_assert(melon::duration::milliseconds(N) == melon::duration::from_chrono(milliseconds(N)), "");
        static_assert(melon::duration::seconds(N) == melon::duration::from_chrono(seconds(N)), "");
        static_assert(melon::duration::minutes(N) == melon::duration::from_chrono(minutes(N)), "");
        static_assert(melon::duration::hours(N) == melon::duration::from_chrono(hours(N)), "");
    }

    TEST(duration, from_chrono) {
        Testfrom_chronoBasicEquality<-123>();
        Testfrom_chronoBasicEquality<-1>();
        Testfrom_chronoBasicEquality<0>();
        Testfrom_chronoBasicEquality<1>();
        Testfrom_chronoBasicEquality<123>();

        // minutes (might, depending on the platform) saturate at +inf.
        const auto chrono_minutes_max = std::chrono::minutes::max();
        const auto minutes_max = melon::duration::from_chrono(chrono_minutes_max);
        const int64_t minutes_max_count = chrono_minutes_max.count();
        if (minutes_max_count > kint64max / 60) {
            EXPECT_EQ(melon::infinite_duration(), minutes_max);
        } else {
            EXPECT_EQ(melon::duration::minutes(minutes_max_count), minutes_max);
        }

        // minutes (might, depending on the platform) saturate at -inf.
        const auto chrono_minutes_min = std::chrono::minutes::min();
        const auto minutes_min = melon::duration::from_chrono(chrono_minutes_min);
        const int64_t minutes_min_count = chrono_minutes_min.count();
        if (minutes_min_count < kint64min / 60) {
            EXPECT_EQ(-melon::infinite_duration(), minutes_min);
        } else {
            EXPECT_EQ(melon::duration::minutes(minutes_min_count), minutes_min);
        }

        // hours (might, depending on the platform) saturate at +inf.
        const auto chrono_hours_max = std::chrono::hours::max();
        const auto hours_max = melon::duration::from_chrono(chrono_hours_max);
        const int64_t hours_max_count = chrono_hours_max.count();
        if (hours_max_count > kint64max / 3600) {
            EXPECT_EQ(melon::infinite_duration(), hours_max);
        } else {
            EXPECT_EQ(melon::duration::hours(hours_max_count), hours_max);
        }

        // hours (might, depending on the platform) saturate at -inf.
        const auto chrono_hours_min = std::chrono::hours::min();
        const auto hours_min = melon::duration::from_chrono(chrono_hours_min);
        const int64_t hours_min_count = chrono_hours_min.count();
        if (hours_min_count < kint64min / 3600) {
            EXPECT_EQ(-melon::infinite_duration(), hours_min);
        } else {
            EXPECT_EQ(melon::duration::hours(hours_min_count), hours_min);
        }
    }

    template<int64_t N>
    void TestToChrono() {
        using std::chrono::nanoseconds;
        using std::chrono::microseconds;
        using std::chrono::milliseconds;
        using std::chrono::seconds;
        using std::chrono::minutes;
        using std::chrono::hours;

        EXPECT_EQ(nanoseconds(N), melon::duration::nanoseconds(N).to_chrono_nanoseconds());
        EXPECT_EQ(microseconds(N), melon::duration::microseconds(N).to_chrono_microseconds());
        EXPECT_EQ(milliseconds(N), melon::duration::milliseconds(N).to_chrono_milliseconds());
        EXPECT_EQ(seconds(N), melon::duration::seconds(N).to_chrono_seconds());

        constexpr auto melon_minutes = melon::duration::minutes(N);
        auto chrono_minutes = minutes(N);
        if (melon_minutes == -melon::infinite_duration()) {
            chrono_minutes = minutes::min();
        } else if (melon_minutes == melon::infinite_duration()) {
            chrono_minutes = minutes::max();
        }
        EXPECT_EQ(chrono_minutes, melon_minutes.to_chrono_minutes());

        constexpr auto melon__hours = melon::duration::hours(N);
        auto chrono_hours = hours(N);
        if (melon__hours == -melon::infinite_duration()) {
            chrono_hours = hours::min();
        } else if (melon__hours == melon::infinite_duration()) {
            chrono_hours = hours::max();
        }
        EXPECT_EQ(chrono_hours, melon__hours.to_chrono_hours());
    }

    TEST(duration, ToChrono) {
        using std::chrono::nanoseconds;
        using std::chrono::microseconds;
        using std::chrono::milliseconds;
        using std::chrono::seconds;
        using std::chrono::minutes;
        using std::chrono::hours;

        TestToChrono<kint64min>();
        TestToChrono<-1>();
        TestToChrono<0>();
        TestToChrono<1>();
        TestToChrono<kint64max>();

        // Verify truncation toward zero.
        const auto tick = melon::duration::nanoseconds(1) / 4;
        EXPECT_EQ(nanoseconds(0), tick.to_chrono_nanoseconds());
        EXPECT_EQ(nanoseconds(0), (-tick).to_chrono_nanoseconds());
        EXPECT_EQ(microseconds(0), tick.to_chrono_microseconds());
        EXPECT_EQ(microseconds(0), (-tick).to_chrono_microseconds());
        EXPECT_EQ(milliseconds(0), tick.to_chrono_milliseconds());
        EXPECT_EQ(milliseconds(0), (-tick).to_chrono_milliseconds());
        EXPECT_EQ(seconds(0), tick.to_chrono_seconds());
        EXPECT_EQ(seconds(0), (-tick).to_chrono_seconds());
        EXPECT_EQ(minutes(0), tick.to_chrono_minutes());
        EXPECT_EQ(minutes(0), (-tick).to_chrono_minutes());
        EXPECT_EQ(hours(0), tick.to_chrono_hours());
        EXPECT_EQ(hours(0), (-tick).to_chrono_hours());

        // Verifies +/- infinity saturation at max/min.
        constexpr auto inf = melon::infinite_duration();
        EXPECT_EQ(nanoseconds::min(), (-inf).to_chrono_nanoseconds());
        EXPECT_EQ(nanoseconds::max(), inf.to_chrono_nanoseconds());
        EXPECT_EQ(microseconds::min(), (-inf).to_chrono_microseconds());
        EXPECT_EQ(microseconds::max(), inf.to_chrono_microseconds());
        EXPECT_EQ(milliseconds::min(), (-inf).to_chrono_milliseconds());
        EXPECT_EQ(milliseconds::max(), inf.to_chrono_milliseconds());
        EXPECT_EQ(seconds::min(), (-inf).to_chrono_seconds());
        EXPECT_EQ(seconds::max(), inf.to_chrono_seconds());
        EXPECT_EQ(minutes::min(), (-inf).to_chrono_minutes());
        EXPECT_EQ(minutes::max(), inf.to_chrono_minutes());
        EXPECT_EQ(hours::min(), (-inf).to_chrono_hours());
        EXPECT_EQ(hours::max(), inf.to_chrono_hours());
    }

    TEST(duration, FactoryOverloads) {
        enum E {
            kOne = 1
        };
#define TEST_FACTORY_OVERLOADS(NAME)                                          \
  EXPECT_EQ(1, NAME(kOne) / NAME(kOne));                                      \
  EXPECT_EQ(1, NAME(static_cast<int8_t>(1)) / NAME(1));                       \
  EXPECT_EQ(1, NAME(static_cast<int16_t>(1)) / NAME(1));                      \
  EXPECT_EQ(1, NAME(static_cast<int32_t>(1)) / NAME(1));                      \
  EXPECT_EQ(1, NAME(static_cast<int64_t>(1)) / NAME(1));                      \
  EXPECT_EQ(1, NAME(static_cast<uint8_t>(1)) / NAME(1));                      \
  EXPECT_EQ(1, NAME(static_cast<uint16_t>(1)) / NAME(1));                     \
  EXPECT_EQ(1, NAME(static_cast<uint32_t>(1)) / NAME(1));                     \
  EXPECT_EQ(1, NAME(static_cast<uint64_t>(1)) / NAME(1));                     \
  EXPECT_EQ(NAME(1) / 2, NAME(static_cast<float>(0.5)));                      \
  EXPECT_EQ(NAME(1) / 2, NAME(static_cast<double>(0.5)));                     \
  EXPECT_EQ(1.5, NAME(static_cast<float>(1.5)).float_div_duration(NAME(1))); \
  EXPECT_EQ(1.5, NAME(static_cast<double>(1.5)).float_div_duration(NAME(1)));

        TEST_FACTORY_OVERLOADS(melon::duration::nanoseconds);
        TEST_FACTORY_OVERLOADS(melon::duration::microseconds);
        TEST_FACTORY_OVERLOADS(melon::duration::milliseconds);
        TEST_FACTORY_OVERLOADS(melon::duration::seconds);
        TEST_FACTORY_OVERLOADS(melon::duration::minutes);
        TEST_FACTORY_OVERLOADS(melon::duration::hours);

#undef TEST_FACTORY_OVERLOADS

        EXPECT_EQ(melon::duration::milliseconds(1500), melon::duration::seconds(1.5));
        EXPECT_LT(melon::duration::nanoseconds(1), melon::duration::nanoseconds(1.5));
        EXPECT_GT(melon::duration::nanoseconds(2), melon::duration::nanoseconds(1.5));

        const double dbl_inf = std::numeric_limits<double>::infinity();
        EXPECT_EQ(melon::infinite_duration(), melon::duration::nanoseconds(dbl_inf));
        EXPECT_EQ(melon::infinite_duration(), melon::duration::microseconds(dbl_inf));
        EXPECT_EQ(melon::infinite_duration(), melon::duration::milliseconds(dbl_inf));
        EXPECT_EQ(melon::infinite_duration(), melon::duration::seconds(dbl_inf));
        EXPECT_EQ(melon::infinite_duration(), melon::duration::minutes(dbl_inf));
        EXPECT_EQ(melon::infinite_duration(), melon::duration::hours(dbl_inf));
        EXPECT_EQ(-melon::infinite_duration(), melon::duration::nanoseconds(-dbl_inf));
        EXPECT_EQ(-melon::infinite_duration(), melon::duration::microseconds(-dbl_inf));
        EXPECT_EQ(-melon::infinite_duration(), melon::duration::milliseconds(-dbl_inf));
        EXPECT_EQ(-melon::infinite_duration(), melon::duration::seconds(-dbl_inf));
        EXPECT_EQ(-melon::infinite_duration(), melon::duration::minutes(-dbl_inf));
        EXPECT_EQ(-melon::infinite_duration(), melon::duration::hours(-dbl_inf));
    }

    TEST(duration, InfinityExamples) {
        // These examples are used in the documentation in time.h. They are
        // written so that they can be copy-n-pasted easily.

        constexpr melon::duration inf = melon::infinite_duration();
        constexpr melon::duration d = melon::duration::seconds(1);  // Any finite duration

        EXPECT_TRUE(inf == inf + inf);
        EXPECT_TRUE(inf == inf + d);
        EXPECT_TRUE(inf == inf - inf);
        EXPECT_TRUE(-inf == d - inf);

        EXPECT_TRUE(inf == d * 1e100);
        EXPECT_TRUE(0 == d / inf);  // NOLINT(readability/check)

        // Division by zero returns infinity, or kint64min/MAX where necessary.
        EXPECT_TRUE(inf == d / 0);
        EXPECT_TRUE(kint64max == d / melon::zero_duration());
    }

    TEST(duration, InfinityComparison) {
        const melon::duration inf = melon::infinite_duration();
        const melon::duration any_dur = melon::duration::seconds(1);

        // Equality
        EXPECT_EQ(inf, inf);
        EXPECT_EQ(-inf, -inf);
        EXPECT_NE(inf, -inf);
        EXPECT_NE(any_dur, inf);
        EXPECT_NE(any_dur, -inf);

        // Relational
        EXPECT_GT(inf, any_dur);
        EXPECT_LT(-inf, any_dur);
        EXPECT_LT(-inf, inf);
        EXPECT_GT(inf, -inf);
    }

    TEST(duration, InfinityAddition) {
        const melon::duration sec_max = melon::duration::seconds(kint64max);
        const melon::duration sec_min = melon::duration::seconds(kint64min);
        const melon::duration any_dur = melon::duration::seconds(1);
        const melon::duration inf = melon::infinite_duration();

        // Addition
        EXPECT_EQ(inf, inf + inf);
        EXPECT_EQ(inf, inf + -inf);
        EXPECT_EQ(-inf, -inf + inf);
        EXPECT_EQ(-inf, -inf + -inf);

        EXPECT_EQ(inf, inf + any_dur);
        EXPECT_EQ(inf, any_dur + inf);
        EXPECT_EQ(-inf, -inf + any_dur);
        EXPECT_EQ(-inf, any_dur + -inf);

        // Interesting case
        melon::duration almost_inf = sec_max + melon::duration::nanoseconds(999999999);
        EXPECT_GT(inf, almost_inf);
        almost_inf += -melon::duration::nanoseconds(999999999);
        EXPECT_GT(inf, almost_inf);

        // Addition overflow/underflow
        EXPECT_EQ(inf, sec_max + melon::duration::seconds(1));
        EXPECT_EQ(inf, sec_max + sec_max);
        EXPECT_EQ(-inf, sec_min + -melon::duration::seconds(1));
        EXPECT_EQ(-inf, sec_min + -sec_max);

        // For reference: IEEE 754 behavior
        const double dbl_inf = std::numeric_limits<double>::infinity();
        EXPECT_TRUE(std::isinf(dbl_inf + dbl_inf));
        EXPECT_TRUE(std::isnan(dbl_inf + -dbl_inf));  // We return inf
        EXPECT_TRUE(std::isnan(-dbl_inf + dbl_inf));  // We return inf
        EXPECT_TRUE(std::isinf(-dbl_inf + -dbl_inf));
    }

    TEST(duration, InfinitySubtraction) {
        const melon::duration sec_max = melon::duration::seconds(kint64max);
        const melon::duration sec_min = melon::duration::seconds(kint64min);
        const melon::duration any_dur = melon::duration::seconds(1);
        const melon::duration inf = melon::infinite_duration();

        // Subtraction
        EXPECT_EQ(inf, inf - inf);
        EXPECT_EQ(inf, inf - -inf);
        EXPECT_EQ(-inf, -inf - inf);
        EXPECT_EQ(-inf, -inf - -inf);

        EXPECT_EQ(inf, inf - any_dur);
        EXPECT_EQ(-inf, any_dur - inf);
        EXPECT_EQ(-inf, -inf - any_dur);
        EXPECT_EQ(inf, any_dur - -inf);

        // Subtraction overflow/underflow
        EXPECT_EQ(inf, sec_max - -melon::duration::seconds(1));
        EXPECT_EQ(inf, sec_max - -sec_max);
        EXPECT_EQ(-inf, sec_min - melon::duration::seconds(1));
        EXPECT_EQ(-inf, sec_min - sec_max);

        // Interesting case
        melon::duration almost_neg_inf = sec_min;
        EXPECT_LT(-inf, almost_neg_inf);
        almost_neg_inf -= -melon::duration::nanoseconds(1);
        EXPECT_LT(-inf, almost_neg_inf);

        // For reference: IEEE 754 behavior
        const double dbl_inf = std::numeric_limits<double>::infinity();
        EXPECT_TRUE(std::isnan(dbl_inf - dbl_inf));  // We return inf
        EXPECT_TRUE(std::isinf(dbl_inf - -dbl_inf));
        EXPECT_TRUE(std::isinf(-dbl_inf - dbl_inf));
        EXPECT_TRUE(std::isnan(-dbl_inf - -dbl_inf));  // We return inf
    }

    TEST(duration, InfinityMultiplication) {
        const melon::duration sec_max = melon::duration::seconds(kint64max);
        const melon::duration sec_min = melon::duration::seconds(kint64min);
        const melon::duration inf = melon::infinite_duration();

#define TEST_INF_MUL_WITH_TYPE(T)                                     \
  EXPECT_EQ(inf, inf * static_cast<T>(2));                            \
  EXPECT_EQ(-inf, inf * static_cast<T>(-2));                          \
  EXPECT_EQ(-inf, -inf * static_cast<T>(2));                          \
  EXPECT_EQ(inf, -inf * static_cast<T>(-2));                          \
  EXPECT_EQ(inf, inf * static_cast<T>(0));                            \
  EXPECT_EQ(-inf, -inf * static_cast<T>(0));                          \
  EXPECT_EQ(inf, sec_max * static_cast<T>(2));                        \
  EXPECT_EQ(inf, sec_min * static_cast<T>(-2));                       \
  EXPECT_EQ(inf, (sec_max / static_cast<T>(2)) * static_cast<T>(3));  \
  EXPECT_EQ(-inf, sec_max * static_cast<T>(-2));                      \
  EXPECT_EQ(-inf, sec_min * static_cast<T>(2));                       \
  EXPECT_EQ(-inf, (sec_min / static_cast<T>(2)) * static_cast<T>(3));

        TEST_INF_MUL_WITH_TYPE(int64_t);  // NOLINT(readability/function)
        TEST_INF_MUL_WITH_TYPE(double);   // NOLINT(readability/function)

#undef TEST_INF_MUL_WITH_TYPE

        const double dbl_inf = std::numeric_limits<double>::infinity();
        EXPECT_EQ(inf, inf * dbl_inf);
        EXPECT_EQ(-inf, -inf * dbl_inf);
        EXPECT_EQ(-inf, inf * -dbl_inf);
        EXPECT_EQ(inf, -inf * -dbl_inf);

        const melon::duration any_dur = melon::duration::seconds(1);
        EXPECT_EQ(inf, any_dur * dbl_inf);
        EXPECT_EQ(-inf, -any_dur * dbl_inf);
        EXPECT_EQ(-inf, any_dur * -dbl_inf);
        EXPECT_EQ(inf, -any_dur * -dbl_inf);

        // Fixed-point multiplication will produce a finite value, whereas floating
        // point fuzziness will overflow to inf.
        EXPECT_NE(melon::infinite_duration(), melon::duration::seconds(1) * kint64max);
        EXPECT_EQ(inf, melon::duration::seconds(1) * static_cast<double>(kint64max));
        EXPECT_NE(-melon::infinite_duration(), melon::duration::seconds(1) * kint64min);
        EXPECT_EQ(-inf, melon::duration::seconds(1) * static_cast<double>(kint64min));

        // Note that sec_max * or / by 1.0 overflows to inf due to the 53-bit
        // limitations of double.
        EXPECT_NE(inf, sec_max);
        EXPECT_NE(inf, sec_max / 1);
        EXPECT_EQ(inf, sec_max / 1.0);
        EXPECT_NE(inf, sec_max * 1);
        EXPECT_EQ(inf, sec_max * 1.0);
    }

    TEST(duration, InfinityDivision) {
        const melon::duration sec_max = melon::duration::seconds(kint64max);
        const melon::duration sec_min = melon::duration::seconds(kint64min);
        const melon::duration inf = melon::infinite_duration();

        // Division of duration by a double
#define TEST_INF_DIV_WITH_TYPE(T)            \
  EXPECT_EQ(inf, inf / static_cast<T>(2));   \
  EXPECT_EQ(-inf, inf / static_cast<T>(-2)); \
  EXPECT_EQ(-inf, -inf / static_cast<T>(2)); \
  EXPECT_EQ(inf, -inf / static_cast<T>(-2));

        TEST_INF_DIV_WITH_TYPE(int64_t);  // NOLINT(readability/function)
        TEST_INF_DIV_WITH_TYPE(double);   // NOLINT(readability/function)

#undef TEST_INF_DIV_WITH_TYPE

        // Division of duration by a double overflow/underflow
        EXPECT_EQ(inf, sec_max / 0.5);
        EXPECT_EQ(inf, sec_min / -0.5);
        EXPECT_EQ(inf, ((sec_max / 0.5) + melon::duration::seconds(1)) / 0.5);
        EXPECT_EQ(-inf, sec_max / -0.5);
        EXPECT_EQ(-inf, sec_min / 0.5);
        EXPECT_EQ(-inf, ((sec_min / 0.5) - melon::duration::seconds(1)) / 0.5);

        const double dbl_inf = std::numeric_limits<double>::infinity();
        EXPECT_EQ(inf, inf / dbl_inf);
        EXPECT_EQ(-inf, inf / -dbl_inf);
        EXPECT_EQ(-inf, -inf / dbl_inf);
        EXPECT_EQ(inf, -inf / -dbl_inf);

        const melon::duration any_dur = melon::duration::seconds(1);
        EXPECT_EQ(melon::zero_duration(), any_dur / dbl_inf);
        EXPECT_EQ(melon::zero_duration(), any_dur / -dbl_inf);
        EXPECT_EQ(melon::zero_duration(), -any_dur / dbl_inf);
        EXPECT_EQ(melon::zero_duration(), -any_dur / -dbl_inf);
    }

    TEST(duration, InfinityModulus) {
        const melon::duration sec_max = melon::duration::seconds(kint64max);
        const melon::duration any_dur = melon::duration::seconds(1);
        const melon::duration inf = melon::infinite_duration();

        EXPECT_EQ(inf, inf % inf);
        EXPECT_EQ(inf, inf % -inf);
        EXPECT_EQ(-inf, -inf % -inf);
        EXPECT_EQ(-inf, -inf % inf);

        EXPECT_EQ(any_dur, any_dur % inf);
        EXPECT_EQ(any_dur, any_dur % -inf);
        EXPECT_EQ(-any_dur, -any_dur % inf);
        EXPECT_EQ(-any_dur, -any_dur % -inf);

        EXPECT_EQ(inf, inf % -any_dur);
        EXPECT_EQ(inf, inf % any_dur);
        EXPECT_EQ(-inf, -inf % -any_dur);
        EXPECT_EQ(-inf, -inf % any_dur);

        // Remainder isn't affected by overflow.
        EXPECT_EQ(melon::zero_duration(), sec_max % melon::duration::seconds(1));
        EXPECT_EQ(melon::zero_duration(), sec_max % melon::duration::milliseconds(1));
        EXPECT_EQ(melon::zero_duration(), sec_max % melon::duration::microseconds(1));
        EXPECT_EQ(melon::zero_duration(), sec_max % melon::duration::nanoseconds(1));
        EXPECT_EQ(melon::zero_duration(), sec_max % melon::duration::nanoseconds(1) / 4);
    }

    TEST(duration, InfinityIDiv) {
        const melon::duration sec_max = melon::duration::seconds(kint64max);
        const melon::duration any_dur = melon::duration::seconds(1);
        const melon::duration inf = melon::infinite_duration();
        const double dbl_inf = std::numeric_limits<double>::infinity();

        // integer_div_duration (int64_t return value + a remainer)
        melon::duration rem = melon::zero_duration();
        EXPECT_EQ(kint64max, melon::duration::integer_div_duration(inf, inf, &rem));
        EXPECT_EQ(inf, rem);

        rem = melon::zero_duration();
        EXPECT_EQ(kint64max, melon::duration::integer_div_duration(-inf, -inf, &rem));
        EXPECT_EQ(-inf, rem);

        rem = melon::zero_duration();
        EXPECT_EQ(kint64max, melon::duration::integer_div_duration(inf, any_dur, &rem));
        EXPECT_EQ(inf, rem);

        rem = melon::zero_duration();
        EXPECT_EQ(0, melon::duration::integer_div_duration(any_dur, inf, &rem));
        EXPECT_EQ(any_dur, rem);

        rem = melon::zero_duration();
        EXPECT_EQ(kint64max, melon::duration::integer_div_duration(-inf, -any_dur, &rem));
        EXPECT_EQ(-inf, rem);

        rem = melon::zero_duration();
        EXPECT_EQ(0, melon::duration::integer_div_duration(-any_dur, -inf, &rem));
        EXPECT_EQ(-any_dur, rem);

        rem = melon::zero_duration();
        EXPECT_EQ(kint64min, melon::duration::integer_div_duration(-inf, inf, &rem));
        EXPECT_EQ(-inf, rem);

        rem = melon::zero_duration();
        EXPECT_EQ(kint64min, melon::duration::integer_div_duration(inf, -inf, &rem));
        EXPECT_EQ(inf, rem);

        rem = melon::zero_duration();
        EXPECT_EQ(kint64min, melon::duration::integer_div_duration(-inf, any_dur, &rem));
        EXPECT_EQ(-inf, rem);

        rem = melon::zero_duration();
        EXPECT_EQ(0, melon::duration::integer_div_duration(-any_dur, inf, &rem));
        EXPECT_EQ(-any_dur, rem);

        rem = melon::zero_duration();
        EXPECT_EQ(kint64min, melon::duration::integer_div_duration(inf, -any_dur, &rem));
        EXPECT_EQ(inf, rem);

        rem = melon::zero_duration();
        EXPECT_EQ(0, melon::duration::integer_div_duration(any_dur, -inf, &rem));
        EXPECT_EQ(any_dur, rem);

        // integer_div_duration overflow/underflow
        rem = any_dur;
        EXPECT_EQ(kint64max,
                  melon::duration::integer_div_duration(sec_max, melon::duration::nanoseconds(1) / 4, &rem));
        EXPECT_EQ(sec_max - melon::duration::nanoseconds(kint64max) / 4, rem);

        rem = any_dur;
        EXPECT_EQ(kint64max,
                  melon::duration::integer_div_duration(sec_max, melon::duration::milliseconds(1), &rem));
        EXPECT_EQ(sec_max - melon::duration::milliseconds(kint64max), rem);

        rem = any_dur;
        EXPECT_EQ(kint64max,
                  melon::duration::integer_div_duration(-sec_max, -melon::duration::milliseconds(1), &rem));
        EXPECT_EQ(-sec_max + melon::duration::milliseconds(kint64max), rem);

        rem = any_dur;
        EXPECT_EQ(kint64min,
                  melon::duration::integer_div_duration(-sec_max, melon::duration::milliseconds(1), &rem));
        EXPECT_EQ(-sec_max - melon::duration::milliseconds(kint64min), rem);

        rem = any_dur;
        EXPECT_EQ(kint64min,
                  melon::duration::integer_div_duration(sec_max, -melon::duration::milliseconds(1), &rem));
        EXPECT_EQ(sec_max + melon::duration::milliseconds(kint64min), rem);

        //
        // operator/(duration, duration) is a wrapper for integer_div_duration().
        //

        // IEEE 754 says inf / inf should be nan, but int64_t doesn't have
        // nan so we'll return kint64max/kint64min instead.
        EXPECT_TRUE(std::isnan(dbl_inf / dbl_inf));
        EXPECT_EQ(kint64max, inf / inf);
        EXPECT_EQ(kint64max, -inf / -inf);
        EXPECT_EQ(kint64min, -inf / inf);
        EXPECT_EQ(kint64min, inf / -inf);

        EXPECT_TRUE(std::isinf(dbl_inf / 2.0));
        EXPECT_EQ(kint64max, inf / any_dur);
        EXPECT_EQ(kint64max, -inf / -any_dur);
        EXPECT_EQ(kint64min, -inf / any_dur);
        EXPECT_EQ(kint64min, inf / -any_dur);

        EXPECT_EQ(0.0, 2.0 / dbl_inf);
        EXPECT_EQ(0, any_dur / inf);
        EXPECT_EQ(0, any_dur / -inf);
        EXPECT_EQ(0, -any_dur / inf);
        EXPECT_EQ(0, -any_dur / -inf);
        EXPECT_EQ(0, melon::zero_duration() / inf);

        // Division of duration by a duration overflow/underflow
        EXPECT_EQ(kint64max, sec_max / melon::duration::milliseconds(1));
        EXPECT_EQ(kint64max, -sec_max / -melon::duration::milliseconds(1));
        EXPECT_EQ(kint64min, -sec_max / melon::duration::milliseconds(1));
        EXPECT_EQ(kint64min, sec_max / -melon::duration::milliseconds(1));
    }

    TEST(duration, InfinityFDiv) {
        const melon::duration any_dur = melon::duration::seconds(1);
        const melon::duration inf = melon::infinite_duration();
        const double dbl_inf = std::numeric_limits<double>::infinity();

        EXPECT_EQ(dbl_inf, inf.float_div_duration(inf));
        EXPECT_EQ(dbl_inf, (-inf).float_div_duration(-inf));
        EXPECT_EQ(dbl_inf, inf.float_div_duration(any_dur));
        EXPECT_EQ(0.0, any_dur.float_div_duration(inf));
        EXPECT_EQ(dbl_inf, (-inf).float_div_duration(-any_dur));
        EXPECT_EQ(0.0, (-any_dur).float_div_duration(-inf));

        EXPECT_EQ(-dbl_inf, (-inf).float_div_duration(inf));
        EXPECT_EQ(-dbl_inf, inf.float_div_duration(-inf));
        EXPECT_EQ(-dbl_inf, (-inf).float_div_duration(any_dur));
        EXPECT_EQ(0.0, (-any_dur).float_div_duration(inf));
        EXPECT_EQ(-dbl_inf, inf.float_div_duration(-any_dur));
        EXPECT_EQ(0.0, any_dur.float_div_duration(-inf));
    }

    TEST(duration, DivisionByZero) {
        const melon::duration zero = melon::zero_duration();
        const melon::duration inf = melon::infinite_duration();
        const melon::duration any_dur = melon::duration::seconds(1);
        const double dbl_inf = std::numeric_limits<double>::infinity();
        const double dbl_denorm = std::numeric_limits<double>::denorm_min();

        // Operator/(duration, double)
        EXPECT_EQ(inf, zero / 0.0);
        EXPECT_EQ(-inf, zero / -0.0);
        EXPECT_EQ(inf, any_dur / 0.0);
        EXPECT_EQ(-inf, any_dur / -0.0);
        EXPECT_EQ(-inf, -any_dur / 0.0);
        EXPECT_EQ(inf, -any_dur / -0.0);

        // Tests dividing by a number very close to, but not quite zero.
        EXPECT_EQ(zero, zero / dbl_denorm);
        EXPECT_EQ(zero, zero / -dbl_denorm);
        EXPECT_EQ(inf, any_dur / dbl_denorm);
        EXPECT_EQ(-inf, any_dur / -dbl_denorm);
        EXPECT_EQ(-inf, -any_dur / dbl_denorm);
        EXPECT_EQ(inf, -any_dur / -dbl_denorm);

        // IDiv
        melon::duration rem = zero;
        EXPECT_EQ(kint64max, melon::duration::integer_div_duration(zero, zero, &rem));
        EXPECT_EQ(inf, rem);

        rem = zero;
        EXPECT_EQ(kint64max, melon::duration::integer_div_duration(any_dur, zero, &rem));
        EXPECT_EQ(inf, rem);

        rem = zero;
        EXPECT_EQ(kint64min, melon::duration::integer_div_duration(-any_dur, zero, &rem));
        EXPECT_EQ(-inf, rem);

        // Operator/(duration, duration)
        EXPECT_EQ(kint64max, zero / zero);
        EXPECT_EQ(kint64max, any_dur / zero);
        EXPECT_EQ(kint64min, -any_dur / zero);

        // FDiv
        EXPECT_EQ(dbl_inf, zero.float_div_duration( zero));
        EXPECT_EQ(dbl_inf, any_dur.float_div_duration( zero));
        EXPECT_EQ(-dbl_inf, (-any_dur).float_div_duration(zero));
    }

    TEST(duration, NaN) {
        // Note that IEEE 754 does not define the behavior of a nan's sign when it is
        // copied, so the code below allows for either + or - infinite_duration.
#define TEST_NAN_HANDLING(NAME, NAN)           \
  do {                                         \
    const auto inf = melon::infinite_duration(); \
    auto x = NAME(NAN);                        \
    EXPECT_TRUE(x == inf || x == -inf);        \
    auto y = NAME(42);                         \
    y *= NAN;                                  \
    EXPECT_TRUE(y == inf || y == -inf);        \
    auto z = NAME(42);                         \
    z /= NAN;                                  \
    EXPECT_TRUE(z == inf || z == -inf);        \
  } while (0)

        const double nan = std::numeric_limits<double>::quiet_NaN();
        TEST_NAN_HANDLING(melon::duration::nanoseconds, nan);
        TEST_NAN_HANDLING(melon::duration::microseconds, nan);
        TEST_NAN_HANDLING(melon::duration::milliseconds, nan);
        TEST_NAN_HANDLING(melon::duration::seconds, nan);
        TEST_NAN_HANDLING(melon::duration::minutes, nan);
        TEST_NAN_HANDLING(melon::duration::hours, nan);

        TEST_NAN_HANDLING(melon::duration::nanoseconds, -nan);
        TEST_NAN_HANDLING(melon::duration::microseconds, -nan);
        TEST_NAN_HANDLING(melon::duration::milliseconds, -nan);
        TEST_NAN_HANDLING(melon::duration::seconds, -nan);
        TEST_NAN_HANDLING(melon::duration::minutes, -nan);
        TEST_NAN_HANDLING(melon::duration::hours, -nan);

#undef TEST_NAN_HANDLING
    }

    TEST(duration, Range) {
        const melon::duration range = ApproxYears(100 * 1e9);
        const melon::duration range_future = range;
        const melon::duration range_past = -range;

        EXPECT_LT(range_future, melon::infinite_duration());
        EXPECT_GT(range_past, -melon::infinite_duration());

        const melon::duration full_range = range_future - range_past;
        EXPECT_GT(full_range, melon::zero_duration());
        EXPECT_LT(full_range, melon::infinite_duration());

        const melon::duration neg_full_range = range_past - range_future;
        EXPECT_LT(neg_full_range, melon::zero_duration());
        EXPECT_GT(neg_full_range, -melon::infinite_duration());

        EXPECT_LT(neg_full_range, full_range);
        EXPECT_EQ(neg_full_range, -full_range);
    }

    TEST(duration, RelationalOperators) {
#define TEST_REL_OPS(UNIT)               \
  static_assert(UNIT(2) == UNIT(2), ""); \
  static_assert(UNIT(1) != UNIT(2), ""); \
  static_assert(UNIT(1) < UNIT(2), "");  \
  static_assert(UNIT(3) > UNIT(2), "");  \
  static_assert(UNIT(1) <= UNIT(2), ""); \
  static_assert(UNIT(2) <= UNIT(2), ""); \
  static_assert(UNIT(3) >= UNIT(2), ""); \
  static_assert(UNIT(2) >= UNIT(2), "");

        TEST_REL_OPS(melon::duration::nanoseconds);
        TEST_REL_OPS(melon::duration::microseconds);
        TEST_REL_OPS(melon::duration::milliseconds);
        TEST_REL_OPS(melon::duration::seconds);
        TEST_REL_OPS(melon::duration::minutes);
        TEST_REL_OPS(melon::duration::hours);

#undef TEST_REL_OPS
    }

    TEST(duration, Addition) {
#define TEST_ADD_OPS(UNIT)                  \
  do {                                      \
    EXPECT_EQ(UNIT(2), UNIT(1) + UNIT(1));  \
    EXPECT_EQ(UNIT(1), UNIT(2) - UNIT(1));  \
    EXPECT_EQ(UNIT(0), UNIT(2) - UNIT(2));  \
    EXPECT_EQ(UNIT(-1), UNIT(1) - UNIT(2)); \
    EXPECT_EQ(UNIT(-2), UNIT(0) - UNIT(2)); \
    EXPECT_EQ(UNIT(-2), UNIT(1) - UNIT(3)); \
    melon::duration a = UNIT(1);             \
    a += UNIT(1);                           \
    EXPECT_EQ(UNIT(2), a);                  \
    a -= UNIT(1);                           \
    EXPECT_EQ(UNIT(1), a);                  \
  } while (0)

        TEST_ADD_OPS(melon::duration::nanoseconds);
        TEST_ADD_OPS(melon::duration::microseconds);
        TEST_ADD_OPS(melon::duration::milliseconds);
        TEST_ADD_OPS(melon::duration::seconds);
        TEST_ADD_OPS(melon::duration::minutes);
        TEST_ADD_OPS(melon::duration::hours);

#undef TEST_ADD_OPS

        EXPECT_EQ(melon::duration::seconds(2), melon::duration::seconds(3) - 2 * melon::duration::milliseconds(500));
        EXPECT_EQ(melon::duration::seconds(2) + melon::duration::milliseconds(500),
                  melon::duration::seconds(3) - melon::duration::milliseconds(500));

        EXPECT_EQ(melon::duration::seconds(1) + melon::duration::milliseconds(998),
                  melon::duration::milliseconds(999) + melon::duration::milliseconds(999));

        EXPECT_EQ(melon::duration::milliseconds(-1),
                  melon::duration::milliseconds(998) - melon::duration::milliseconds(999));

        // Tests fractions of a nanoseconds. These are implementation details only.
        EXPECT_GT(melon::duration::nanoseconds(1), melon::duration::nanoseconds(1) / 2);
        EXPECT_EQ(melon::duration::nanoseconds(1),
                  melon::duration::nanoseconds(1) / 2 + melon::duration::nanoseconds(1) / 2);
        EXPECT_GT(melon::duration::nanoseconds(1) / 4, melon::duration::nanoseconds(0));
        EXPECT_EQ(melon::duration::nanoseconds(1) / 8, melon::duration::nanoseconds(0));

        // Tests subtraction that will cause wrap around of the rep_lo_ bits.
        melon::duration d_7_5 = melon::duration::seconds(7) + melon::duration::milliseconds(500);
        melon::duration d_3_7 = melon::duration::seconds(3) + melon::duration::milliseconds(700);
        melon::duration ans_3_8 = melon::duration::seconds(3) + melon::duration::milliseconds(800);
        EXPECT_EQ(ans_3_8, d_7_5 - d_3_7);

        // Subtracting min_duration
        melon::duration min_dur = melon::duration::seconds(kint64min);
        EXPECT_EQ(melon::duration::seconds(0), min_dur - min_dur);
        EXPECT_EQ(melon::duration::seconds(kint64max), melon::duration::seconds(-1) - min_dur);
    }

    TEST(duration, Negation) {
        // By storing negations of various values in constexpr variables we
        // verify that the initializers are constant expressions.
        constexpr melon::duration negated_zero_duration = -melon::zero_duration();
        EXPECT_EQ(negated_zero_duration, melon::zero_duration());

        constexpr melon::duration negated_infinite_duration =
                -melon::infinite_duration();
        EXPECT_NE(negated_infinite_duration, melon::infinite_duration());
        EXPECT_EQ(-negated_infinite_duration, melon::infinite_duration());

        // The public APIs to check if a duration is infinite depend on using
        // -infinite_duration(), but we're trying to test operator- here, so we
        // need to use the lower-level internal query is_infinite_duration.
        EXPECT_TRUE(
                negated_infinite_duration.is_infinite_duration());

        // The largest duration is kint64max seconds and kTicksPerSecond - 1 ticks.
        // Using the melon::times_internal::make_duration API is the cleanest way to
        // construct that duration.
        constexpr melon::duration max_duration = melon::duration::make_duration(
                kint64max, melon::times_internal::kTicksPerSecond - 1);
        constexpr melon::duration negated_max_duration = -max_duration;
        // The largest negatable value is one tick above the minimum representable;
        // it's the negation of max_duration.
        constexpr melon::duration nearly_min_duration =
                melon::duration::make_duration(kint64min, int64_t{1});
        constexpr melon::duration negated_nearly_min_duration = -nearly_min_duration;

        EXPECT_EQ(negated_max_duration, nearly_min_duration);
        EXPECT_EQ(negated_nearly_min_duration, max_duration);
        EXPECT_EQ(-(-max_duration), max_duration);

        constexpr melon::duration min_duration =
                melon::duration::make_duration(kint64min);
        constexpr melon::duration negated_min_duration = -min_duration;
        EXPECT_EQ(negated_min_duration, melon::infinite_duration());
    }

    TEST(duration, AbsoluteValue) {
        EXPECT_EQ(melon::zero_duration(), abs_duration(melon::zero_duration()));
        EXPECT_EQ(melon::duration::seconds(1), abs_duration(melon::duration::seconds(1)));
        EXPECT_EQ(melon::duration::seconds(1), abs_duration(melon::duration::seconds(-1)));

        EXPECT_EQ(melon::infinite_duration(), abs_duration(melon::infinite_duration()));
        EXPECT_EQ(melon::infinite_duration(), abs_duration(-melon::infinite_duration()));

        melon::duration max_dur =
                melon::duration::seconds(kint64max) + (melon::duration::seconds(1) - melon::duration::nanoseconds(1) / 4);
        EXPECT_EQ(max_dur, abs_duration(max_dur));

        melon::duration min_dur = melon::duration::seconds(kint64min);
        EXPECT_EQ(melon::infinite_duration(), abs_duration(min_dur));
        EXPECT_EQ(max_dur, abs_duration(min_dur + melon::duration::nanoseconds(1) / 4));
    }

    TEST(duration, Multiplication) {
#define TEST_MUL_OPS(UNIT)                                    \
  do {                                                        \
    EXPECT_EQ(UNIT(5), UNIT(2) * 2.5);                        \
    EXPECT_EQ(UNIT(2), UNIT(5) / 2.5);                        \
    EXPECT_EQ(UNIT(-5), UNIT(-2) * 2.5);                      \
    EXPECT_EQ(UNIT(-5), -UNIT(2) * 2.5);                      \
    EXPECT_EQ(UNIT(-5), UNIT(2) * -2.5);                      \
    EXPECT_EQ(UNIT(-2), UNIT(-5) / 2.5);                      \
    EXPECT_EQ(UNIT(-2), -UNIT(5) / 2.5);                      \
    EXPECT_EQ(UNIT(-2), UNIT(5) / -2.5);                      \
    EXPECT_EQ(UNIT(2), UNIT(11) % UNIT(3));                   \
    melon::duration a = UNIT(2);                               \
    a *= 2.5;                                                 \
    EXPECT_EQ(UNIT(5), a);                                    \
    a /= 2.5;                                                 \
    EXPECT_EQ(UNIT(2), a);                                    \
    a %= UNIT(1);                                             \
    EXPECT_EQ(UNIT(0), a);                                    \
    melon::duration big = UNIT(1000000000);                    \
    big *= 3;                                                 \
    big /= 3;                                                 \
    EXPECT_EQ(UNIT(1000000000), big);                         \
    EXPECT_EQ(-UNIT(2), -UNIT(2));                            \
    EXPECT_EQ(-UNIT(2), UNIT(2) * -1);                        \
    EXPECT_EQ(-UNIT(2), -1 * UNIT(2));                        \
    EXPECT_EQ(-UNIT(-2), UNIT(2));                            \
    EXPECT_EQ(2, UNIT(2) / UNIT(1));                          \
    melon::duration rem;                                       \
    EXPECT_EQ(2, melon::duration::integer_div_duration(UNIT(2), UNIT(1), &rem)); \
    EXPECT_EQ(2.0, UNIT(2).float_div_duration(UNIT(1)));     \
  } while (0)

        TEST_MUL_OPS(melon::duration::nanoseconds);
        TEST_MUL_OPS(melon::duration::microseconds);
        TEST_MUL_OPS(melon::duration::milliseconds);
        TEST_MUL_OPS(melon::duration::seconds);
        TEST_MUL_OPS(melon::duration::minutes);
        TEST_MUL_OPS(melon::duration::hours);

#undef TEST_MUL_OPS

        // Ensures that multiplication and division by 1 with a maxed-out durations
        // doesn't lose precision.
        melon::duration max_dur =
                melon::duration::seconds(kint64max) + (melon::duration::seconds(1) - melon::duration::nanoseconds(1) / 4);
        melon::duration min_dur = melon::duration::seconds(kint64min);
        EXPECT_EQ(max_dur, max_dur * 1);
        EXPECT_EQ(max_dur, max_dur / 1);
        EXPECT_EQ(min_dur, min_dur * 1);
        EXPECT_EQ(min_dur, min_dur / 1);

        // Tests division on a duration with a large number of significant digits.
        // Tests when the digits span hi and lo as well as only in hi.
        melon::duration sigfigs = melon::duration::seconds(2000000000) + melon::duration::nanoseconds(3);
        EXPECT_EQ(melon::duration::seconds(666666666) + melon::duration::nanoseconds(666666667) +
                  melon::duration::nanoseconds(1) / 2,
                  sigfigs / 3);
        sigfigs = melon::duration::seconds(7000000000LL);
        EXPECT_EQ(melon::duration::seconds(2333333333) + melon::duration::nanoseconds(333333333) +
                  melon::duration::nanoseconds(1) / 4,
                  sigfigs / 3);

        EXPECT_EQ(melon::duration::seconds(7) + melon::duration::milliseconds(500), melon::duration::seconds(3) * 2.5);
        EXPECT_EQ(melon::duration::seconds(8) * -1 + melon::duration::milliseconds(300),
                  (melon::duration::seconds(2) + melon::duration::milliseconds(200)) * -3.5);
        EXPECT_EQ(-melon::duration::seconds(8) + melon::duration::milliseconds(300),
                  (melon::duration::seconds(2) + melon::duration::milliseconds(200)) * -3.5);
        EXPECT_EQ(melon::duration::seconds(1) + melon::duration::milliseconds(875),
                  (melon::duration::seconds(7) + melon::duration::milliseconds(500)) / 4);
        EXPECT_EQ(melon::duration::seconds(30),
                  (melon::duration::seconds(7) + melon::duration::milliseconds(500)) / 0.25);
        EXPECT_EQ(melon::duration::seconds(3),
                  (melon::duration::seconds(7) + melon::duration::milliseconds(500)) / 2.5);

        // Tests division remainder.
        EXPECT_EQ(melon::duration::nanoseconds(0), melon::duration::nanoseconds(7) % melon::duration::nanoseconds(1));
        EXPECT_EQ(melon::duration::nanoseconds(0), melon::duration::nanoseconds(0) % melon::duration::nanoseconds(10));
        EXPECT_EQ(melon::duration::nanoseconds(2), melon::duration::nanoseconds(7) % melon::duration::nanoseconds(5));
        EXPECT_EQ(melon::duration::nanoseconds(2), melon::duration::nanoseconds(2) % melon::duration::nanoseconds(5));

        EXPECT_EQ(melon::duration::nanoseconds(1), melon::duration::nanoseconds(10) % melon::duration::nanoseconds(3));
        EXPECT_EQ(melon::duration::nanoseconds(1),
                  melon::duration::nanoseconds(10) % melon::duration::nanoseconds(-3));
        EXPECT_EQ(melon::duration::nanoseconds(-1),
                  melon::duration::nanoseconds(-10) % melon::duration::nanoseconds(3));
        EXPECT_EQ(melon::duration::nanoseconds(-1),
                  melon::duration::nanoseconds(-10) % melon::duration::nanoseconds(-3));

        EXPECT_EQ(melon::duration::milliseconds(100),
                  melon::duration::seconds(1) % melon::duration::milliseconds(300));
        EXPECT_EQ(
                melon::duration::milliseconds(300),
                (melon::duration::seconds(3) + melon::duration::milliseconds(800)) % melon::duration::milliseconds(500));

        EXPECT_EQ(melon::duration::nanoseconds(1), melon::duration::nanoseconds(1) % melon::duration::seconds(1));
        EXPECT_EQ(melon::duration::nanoseconds(-1), melon::duration::nanoseconds(-1) % melon::duration::seconds(1));
        EXPECT_EQ(0, melon::duration::nanoseconds(-1) / melon::duration::seconds(1));  // Actual -1e-9

        // Tests identity a = (a/b)*b + a%b
#define TEST_MOD_IDENTITY(a, b) \
  EXPECT_EQ((a), ((a) / (b))*(b) + ((a)%(b)))

        TEST_MOD_IDENTITY(melon::duration::seconds(0), melon::duration::seconds(2));
        TEST_MOD_IDENTITY(melon::duration::seconds(1), melon::duration::seconds(1));
        TEST_MOD_IDENTITY(melon::duration::seconds(1), melon::duration::seconds(2));
        TEST_MOD_IDENTITY(melon::duration::seconds(2), melon::duration::seconds(1));

        TEST_MOD_IDENTITY(melon::duration::seconds(-2), melon::duration::seconds(1));
        TEST_MOD_IDENTITY(melon::duration::seconds(2), melon::duration::seconds(-1));
        TEST_MOD_IDENTITY(melon::duration::seconds(-2), melon::duration::seconds(-1));

        TEST_MOD_IDENTITY(melon::duration::nanoseconds(0), melon::duration::nanoseconds(2));
        TEST_MOD_IDENTITY(melon::duration::nanoseconds(1), melon::duration::nanoseconds(1));
        TEST_MOD_IDENTITY(melon::duration::nanoseconds(1), melon::duration::nanoseconds(2));
        TEST_MOD_IDENTITY(melon::duration::nanoseconds(2), melon::duration::nanoseconds(1));

        TEST_MOD_IDENTITY(melon::duration::nanoseconds(-2), melon::duration::nanoseconds(1));
        TEST_MOD_IDENTITY(melon::duration::nanoseconds(2), melon::duration::nanoseconds(-1));
        TEST_MOD_IDENTITY(melon::duration::nanoseconds(-2), melon::duration::nanoseconds(-1));

        // Mixed seconds + subseconds
        melon::duration mixed_a = melon::duration::seconds(1) + melon::duration::nanoseconds(2);
        melon::duration mixed_b = melon::duration::seconds(1) + melon::duration::nanoseconds(3);

        TEST_MOD_IDENTITY(melon::duration::seconds(0), mixed_a);
        TEST_MOD_IDENTITY(mixed_a, mixed_a);
        TEST_MOD_IDENTITY(mixed_a, mixed_b);
        TEST_MOD_IDENTITY(mixed_b, mixed_a);

        TEST_MOD_IDENTITY(-mixed_a, mixed_b);
        TEST_MOD_IDENTITY(mixed_a, -mixed_b);
        TEST_MOD_IDENTITY(-mixed_a, -mixed_b);

#undef TEST_MOD_IDENTITY
    }

    TEST(duration, Truncation) {
        const melon::duration d = melon::duration::nanoseconds(1234567890);
        const melon::duration inf = melon::infinite_duration();
        for (int unit_sign : {1, -1}) {  // sign shouldn't matter
            EXPECT_EQ(melon::duration::nanoseconds(1234567890),
                      d.trunc(unit_sign * melon::duration::nanoseconds(1)));
            EXPECT_EQ(melon::duration::microseconds(1234567),
                      d.trunc(unit_sign * melon::duration::microseconds(1)));
            EXPECT_EQ(melon::duration::milliseconds(1234),
                      d.trunc(unit_sign * melon::duration::milliseconds(1)));
            EXPECT_EQ(melon::duration::seconds(1), d.trunc( unit_sign * melon::duration::seconds(1)));
            EXPECT_EQ(inf, inf.trunc(unit_sign * melon::duration::seconds(1)));

            EXPECT_EQ(melon::duration::nanoseconds(-1234567890),
                      (-d).trunc(unit_sign * melon::duration::nanoseconds(1)));
            EXPECT_EQ(melon::duration::microseconds(-1234567),
                      (-d).trunc(unit_sign * melon::duration::microseconds(1)));
            EXPECT_EQ(melon::duration::milliseconds(-1234),
                      (-d).trunc(unit_sign * melon::duration::milliseconds(1)));
            EXPECT_EQ(melon::duration::seconds(-1), (-d).trunc(unit_sign * melon::duration::seconds(1)));
            EXPECT_EQ(-inf, (-inf).trunc(unit_sign * melon::duration::seconds(1)));
        }
    }

    TEST(duration, Flooring) {
        const melon::duration d = melon::duration::nanoseconds(1234567890);
        const melon::duration inf = melon::infinite_duration();
        for (int unit_sign : {1, -1}) {  // sign shouldn't matter
            EXPECT_EQ(melon::duration::nanoseconds(1234567890),
                      d.floor(unit_sign * melon::duration::nanoseconds(1)));
            EXPECT_EQ(melon::duration::microseconds(1234567),
                      d.floor(unit_sign * melon::duration::microseconds(1)));
            EXPECT_EQ(melon::duration::milliseconds(1234),
                      d.floor(unit_sign * melon::duration::milliseconds(1)));
            EXPECT_EQ(melon::duration::seconds(1), d.floor(unit_sign * melon::duration::seconds(1)));
            EXPECT_EQ(inf, inf.floor(unit_sign * melon::duration::seconds(1)));

            EXPECT_EQ(melon::duration::nanoseconds(-1234567890),
                      (-d).floor(unit_sign * melon::duration::nanoseconds(1)));
            EXPECT_EQ(melon::duration::microseconds(-1234568),
                      (-d).floor(unit_sign * melon::duration::microseconds(1)));
            EXPECT_EQ(melon::duration::milliseconds(-1235),
                      (-d).floor(unit_sign * melon::duration::milliseconds(1)));
            EXPECT_EQ(melon::duration::seconds(-2), (-d).floor(unit_sign * melon::duration::seconds(1)));
            EXPECT_EQ(-inf, (-inf).floor(unit_sign * melon::duration::seconds(1)));
        }
    }

    TEST(duration, Ceiling) {
        const melon::duration d = melon::duration::nanoseconds(1234567890);
        const melon::duration inf = melon::infinite_duration();
        for (int unit_sign : {1, -1}) {  // // sign shouldn't matter
            EXPECT_EQ(melon::duration::nanoseconds(1234567890),
                      d.ceil(unit_sign * melon::duration::nanoseconds(1)));
            EXPECT_EQ(melon::duration::microseconds(1234568),
                      d.ceil(unit_sign * melon::duration::microseconds(1)));
            EXPECT_EQ(melon::duration::milliseconds(1235),
                      d.ceil(unit_sign * melon::duration::milliseconds(1)));
            EXPECT_EQ(melon::duration::seconds(2), d.ceil( unit_sign * melon::duration::seconds(1)));
            EXPECT_EQ(inf, inf.ceil(unit_sign * melon::duration::seconds(1)));

            EXPECT_EQ(melon::duration::nanoseconds(-1234567890),
                      (-d).ceil(unit_sign * melon::duration::nanoseconds(1)));
            EXPECT_EQ(melon::duration::microseconds(-1234567),
                      (-d).ceil(unit_sign * melon::duration::microseconds(1)));
            EXPECT_EQ(melon::duration::milliseconds(-1234),
                      (-d).ceil(unit_sign * melon::duration::milliseconds(1)));
            EXPECT_EQ(melon::duration::seconds(-1), (-d).ceil(unit_sign * melon::duration::seconds(1)));
            EXPECT_EQ(-inf, (-inf).ceil(unit_sign * melon::duration::seconds(1)));
        }
    }

    TEST(duration, RoundTripUnits) {
        const int kRange = 100000;

#define ROUND_TRIP_UNIT(U, LOW, HIGH)          \
  do {                                         \
    for (int64_t i = LOW; i < HIGH; ++i) {     \
      melon::duration d = melon::duration::U(i);           \
      if (d == melon::infinite_duration())       \
        EXPECT_EQ(kint64max, d / melon::duration::U(1));  \
      else if (d == -melon::infinite_duration()) \
        EXPECT_EQ(kint64min, d / melon::duration::U(1));  \
      else                                     \
        EXPECT_EQ(i, melon::duration::U(i) / melon::duration::U(1)); \
    }                                          \
  } while (0)

        ROUND_TRIP_UNIT(nanoseconds, kint64min, kint64min + kRange);
        ROUND_TRIP_UNIT(nanoseconds, -kRange, kRange);
        ROUND_TRIP_UNIT(nanoseconds, kint64max - kRange, kint64max);

        ROUND_TRIP_UNIT(microseconds, kint64min, kint64min + kRange);
        ROUND_TRIP_UNIT(microseconds, -kRange, kRange);
        ROUND_TRIP_UNIT(microseconds, kint64max - kRange, kint64max);

        ROUND_TRIP_UNIT(milliseconds, kint64min, kint64min + kRange);
        ROUND_TRIP_UNIT(milliseconds, -kRange, kRange);
        ROUND_TRIP_UNIT(milliseconds, kint64max - kRange, kint64max);

        ROUND_TRIP_UNIT(seconds, kint64min, kint64min + kRange);
        ROUND_TRIP_UNIT(seconds, -kRange, kRange);
        ROUND_TRIP_UNIT(seconds, kint64max - kRange, kint64max);

        ROUND_TRIP_UNIT(minutes, kint64min / 60, kint64min / 60 + kRange);
        ROUND_TRIP_UNIT(minutes, -kRange, kRange);
        ROUND_TRIP_UNIT(minutes, kint64max / 60 - kRange, kint64max / 60);

        ROUND_TRIP_UNIT(hours, kint64min / 3600, kint64min / 3600 + kRange);
        ROUND_TRIP_UNIT(hours, -kRange, kRange);
        ROUND_TRIP_UNIT(hours, kint64max / 3600 - kRange, kint64max / 3600);

#undef ROUND_TRIP_UNIT
    }

    TEST(duration, TruncConversions) {
        // Tests to_timespec()/duration_from_timespec()
        const struct {
            melon::duration d;
            timespec ts;
        } to_ts[] = {
                {melon::duration::seconds(1) + melon::duration::nanoseconds(1),      {1,  1}},
                {melon::duration::seconds(1) + melon::duration::nanoseconds(1) / 2,  {1,  0}},
                {melon::duration::seconds(1) + melon::duration::nanoseconds(0),      {1,  0}},
                {melon::duration::seconds(0) + melon::duration::nanoseconds(0),      {0,  0}},
                {melon::duration::seconds(0) - melon::duration::nanoseconds(1) / 2,  {0,  0}},
                {melon::duration::seconds(0) - melon::duration::nanoseconds(1),      {-1, 999999999}},
                {melon::duration::seconds(-1) + melon::duration::nanoseconds(1),     {-1, 1}},
                {melon::duration::seconds(-1) + melon::duration::nanoseconds(1) / 2, {-1, 1}},
                {melon::duration::seconds(-1) + melon::duration::nanoseconds(0),     {-1, 0}},
                {melon::duration::seconds(-1) - melon::duration::nanoseconds(1) / 2, {-1, 0}},
        };
        for (const auto &test : to_ts) {
            EXPECT_THAT(test.d.to_timespec(), TimespecMatcher(test.ts));
        }
        const struct {
            timespec ts;
            melon::duration d;
        } from_ts[] = {
                {{1,  1},         melon::duration::seconds(1) + melon::duration::nanoseconds(1)},
                {{1,  0},         melon::duration::seconds(1) + melon::duration::nanoseconds(0)},
                {{0,  0},         melon::duration::seconds(0) + melon::duration::nanoseconds(0)},
                {{0,  -1},        melon::duration::seconds(0) - melon::duration::nanoseconds(1)},
                {{-1, 999999999}, melon::duration::seconds(0) - melon::duration::nanoseconds(1)},
                {{-1, 1},         melon::duration::seconds(-1) + melon::duration::nanoseconds(1)},
                {{-1, 0},         melon::duration::seconds(-1) + melon::duration::nanoseconds(0)},
                {{-1, -1},        melon::duration::seconds(-1) - melon::duration::nanoseconds(1)},
                {{-2, 999999999}, melon::duration::seconds(-1) - melon::duration::nanoseconds(1)},
        };
        for (const auto &test : from_ts) {
            EXPECT_EQ(test.d, melon::duration::from_timespec(test.ts));
        }

        // Tests to_timeval()/duration_from_timeval() (same as timespec above)
        const struct {
            melon::duration d;
            timeval tv;
        } to_tv[] = {
                {melon::duration::seconds(1) + melon::duration::microseconds(1),      {1,  1}},
                {melon::duration::seconds(1) + melon::duration::microseconds(1) / 2,  {1,  0}},
                {melon::duration::seconds(1) + melon::duration::microseconds(0),      {1,  0}},
                {melon::duration::seconds(0) + melon::duration::microseconds(0),      {0,  0}},
                {melon::duration::seconds(0) - melon::duration::microseconds(1) / 2,  {0,  0}},
                {melon::duration::seconds(0) - melon::duration::microseconds(1),      {-1, 999999}},
                {melon::duration::seconds(-1) + melon::duration::microseconds(1),     {-1, 1}},
                {melon::duration::seconds(-1) + melon::duration::microseconds(1) / 2, {-1, 1}},
                {melon::duration::seconds(-1) + melon::duration::microseconds(0),     {-1, 0}},
                {melon::duration::seconds(-1) - melon::duration::microseconds(1) / 2, {-1, 0}},
        };
        for (const auto &test : to_tv) {
            EXPECT_THAT(test.d.to_timeval(), TimevalMatcher(test.tv));
        }
        const struct {
            timeval tv;
            melon::duration d;
        } from_tv[] = {
                {{1,  1},      melon::duration::seconds(1) + melon::duration::microseconds(1)},
                {{1,  0},      melon::duration::seconds(1) + melon::duration::microseconds(0)},
                {{0,  0},      melon::duration::seconds(0) + melon::duration::microseconds(0)},
                {{0,  -1},     melon::duration::seconds(0) - melon::duration::microseconds(1)},
                {{-1, 999999}, melon::duration::seconds(0) - melon::duration::microseconds(1)},
                {{-1, 1},      melon::duration::seconds(-1) + melon::duration::microseconds(1)},
                {{-1, 0},      melon::duration::seconds(-1) + melon::duration::microseconds(0)},
                {{-1, -1},     melon::duration::seconds(-1) - melon::duration::microseconds(1)},
                {{-2, 999999}, melon::duration::seconds(-1) - melon::duration::microseconds(1)},
        };
        for (const auto &test : from_tv) {
            EXPECT_EQ(test.d, melon::duration::from_timeval(test.tv));
        }
    }

    TEST(duration, SmallConversions) {
        // Special tests for conversions of small durations.

        EXPECT_EQ(melon::zero_duration(), melon::duration::seconds(0));
        // TODO(bww): Is the next one OK?
        EXPECT_EQ(melon::zero_duration(), melon::duration::seconds(0.124999999e-9));
        EXPECT_EQ(melon::duration::nanoseconds(1) / 4, melon::duration::seconds(0.125e-9));
        EXPECT_EQ(melon::duration::nanoseconds(1) / 4, melon::duration::seconds(0.250e-9));
        EXPECT_EQ(melon::duration::nanoseconds(1) / 2, melon::duration::seconds(0.375e-9));
        EXPECT_EQ(melon::duration::nanoseconds(1) / 2, melon::duration::seconds(0.500e-9));
        EXPECT_EQ(melon::duration::nanoseconds(3) / 4, melon::duration::seconds(0.625e-9));
        EXPECT_EQ(melon::duration::nanoseconds(3) / 4, melon::duration::seconds(0.750e-9));
        EXPECT_EQ(melon::duration::nanoseconds(1), melon::duration::seconds(0.875e-9));
        EXPECT_EQ(melon::duration::nanoseconds(1), melon::duration::seconds(1.000e-9));

        EXPECT_EQ(melon::zero_duration(), melon::duration::seconds(-0.124999999e-9));
        EXPECT_EQ(-melon::duration::nanoseconds(1) / 4, melon::duration::seconds(-0.125e-9));
        EXPECT_EQ(-melon::duration::nanoseconds(1) / 4, melon::duration::seconds(-0.250e-9));
        EXPECT_EQ(-melon::duration::nanoseconds(1) / 2, melon::duration::seconds(-0.375e-9));
        EXPECT_EQ(-melon::duration::nanoseconds(1) / 2, melon::duration::seconds(-0.500e-9));
        EXPECT_EQ(-melon::duration::nanoseconds(3) / 4, melon::duration::seconds(-0.625e-9));
        EXPECT_EQ(-melon::duration::nanoseconds(3) / 4, melon::duration::seconds(-0.750e-9));
        EXPECT_EQ(-melon::duration::nanoseconds(1), melon::duration::seconds(-0.875e-9));
        EXPECT_EQ(-melon::duration::nanoseconds(1), melon::duration::seconds(-1.000e-9));

        timespec ts;
        ts.tv_sec = 0;
        ts.tv_nsec = 0;
        EXPECT_THAT(melon::duration::nanoseconds(0).to_timespec(), TimespecMatcher(ts));
        // TODO(bww): Are the next three OK?
        EXPECT_THAT((melon::duration::nanoseconds(1) / 4).to_timespec(), TimespecMatcher(ts));
        EXPECT_THAT((melon::duration::nanoseconds(2) / 4).to_timespec(), TimespecMatcher(ts));
        EXPECT_THAT((melon::duration::nanoseconds(3) / 4).to_timespec(), TimespecMatcher(ts));
        ts.tv_nsec = 1;
        EXPECT_THAT((melon::duration::nanoseconds(4) / 4).to_timespec(), TimespecMatcher(ts));
        EXPECT_THAT((melon::duration::nanoseconds(5) / 4).to_timespec(), TimespecMatcher(ts));
        EXPECT_THAT((melon::duration::nanoseconds(6) / 4).to_timespec(), TimespecMatcher(ts));
        EXPECT_THAT((melon::duration::nanoseconds(7) / 4).to_timespec(), TimespecMatcher(ts));
        ts.tv_nsec = 2;
        EXPECT_THAT((melon::duration::nanoseconds(8) / 4).to_timespec(), TimespecMatcher(ts));

        timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 0;
        EXPECT_THAT((melon::duration::nanoseconds(0)).to_timeval(), TimevalMatcher(tv));
        // TODO(bww): Is the next one OK?
        EXPECT_THAT((melon::duration::nanoseconds(999)).to_timeval(), TimevalMatcher(tv));
        tv.tv_usec = 1;
        EXPECT_THAT((melon::duration::nanoseconds(1000)).to_timeval(), TimevalMatcher(tv));
        EXPECT_THAT((melon::duration::nanoseconds(1999)).to_timeval(), TimevalMatcher(tv));
        tv.tv_usec = 2;
        EXPECT_THAT((melon::duration::nanoseconds(2000)).to_timeval(), TimevalMatcher(tv));
    }

    void VerifySameAsMul(double time_as_seconds, int *const misses) {
        auto direct_seconds = melon::duration::seconds(time_as_seconds);
        auto mul_by_one_second = time_as_seconds * melon::duration::seconds(1);
        if (direct_seconds != mul_by_one_second) {
            if (*misses > 10)
                return;
            ASSERT_LE(++(*misses), 10) << "Too many errors, not reporting more.";
            EXPECT_EQ(direct_seconds, mul_by_one_second)
                                << "given double time_as_seconds = " << std::setprecision(17)
                                << time_as_seconds;
        }
    }

// For a variety of interesting durations, we find the exact point
// where one double converts to that duration, and the very next double
// converts to the next duration.  For both of those points, verify that
// seconds(point) returns the same duration as point * seconds(1.0)
    TEST(duration, ToDoubleSecondsCheckEdgeCases) {
        constexpr uint32_t kTicksPerSecond = melon::times_internal::kTicksPerSecond;
        constexpr auto duration_tick = melon::duration::make_duration(0, 1u);
        int misses = 0;
        for (int64_t seconds = 0; seconds < 99; ++seconds) {
            uint32_t tick_vals[] = {0, +999, +999999, +999999999, kTicksPerSecond - 1,
                                    0, 1000, 1000000, 1000000000, kTicksPerSecond,
                                    1, 1001, 1000001, 1000000001, kTicksPerSecond + 1,
                                    2, 1002, 1000002, 1000000002, kTicksPerSecond + 2,
                                    3, 1003, 1000003, 1000000003, kTicksPerSecond + 3,
                                    4, 1004, 1000004, 1000000004, kTicksPerSecond + 4,
                                    5, 6, 7, 8, 9};
            for (uint32_t ticks : tick_vals) {
                melon::duration s_plus_t = melon::duration::seconds(seconds) + ticks * duration_tick;
                for (melon::duration d : {s_plus_t, -s_plus_t}) {
                    melon::duration after_d = d + duration_tick;
                    EXPECT_NE(d, after_d);
                    EXPECT_EQ(after_d - d, duration_tick);

                    double low_edge = d.to_double_seconds();
                    EXPECT_EQ(d, melon::duration::seconds(low_edge));

                    double high_edge = after_d.to_double_seconds();
                    EXPECT_EQ(after_d, melon::duration::seconds(high_edge));

                    for (;;) {
                        double midpoint = low_edge + (high_edge - low_edge) / 2;
                        if (midpoint == low_edge || midpoint == high_edge)
                            break;
                        melon::duration mid_duration = melon::duration::seconds(midpoint);
                        if (mid_duration == d) {
                            low_edge = midpoint;
                        } else {
                            EXPECT_EQ(mid_duration, after_d);
                            high_edge = midpoint;
                        }
                    }
                    // Now low_edge is the highest double that converts to duration d,
                    // and high_edge is the lowest double that converts to duration after_d.
                    VerifySameAsMul(low_edge, &misses);
                    VerifySameAsMul(high_edge, &misses);
                }
            }
        }
    }

    TEST(duration, ToDoubleSecondsCheckRandom) {
        std::random_device rd;
        std::seed_seq seed({rd(), rd(), rd(), rd(), rd(), rd(), rd(), rd()});
        std::mt19937_64 gen(seed);
        // We want doubles distributed from 1/8ns up to 2^63, where
        // as many values are tested from 1ns to 2ns as from 1sec to 2sec,
        // so even distribute along a log-scale of those values, and
        // exponentiate before using them.  (9.223377e+18 is just slightly
        // out of bounds for melon::duration.)
        std::uniform_real_distribution<double> uniform(std::log(0.125e-9),
                                                       std::log(9.223377e+18));
        int misses = 0;
        for (int i = 0; i < 1000000; ++i) {
            double d = std::exp(uniform(gen));
            VerifySameAsMul(d, &misses);
            VerifySameAsMul(-d, &misses);
        }
    }

    TEST(duration, ConversionSaturation) {
        melon::duration d;

        const auto max_timeval_sec =
                std::numeric_limits<decltype(timeval::tv_sec)>::max();
        const auto min_timeval_sec =
                std::numeric_limits<decltype(timeval::tv_sec)>::min();
        timeval tv;
        tv.tv_sec = max_timeval_sec;
        tv.tv_usec = 999998;
        d = melon::duration::from_timeval(tv);
        tv = d.to_timeval();
        EXPECT_EQ(max_timeval_sec, tv.tv_sec);
        EXPECT_EQ(999998, tv.tv_usec);
        d += melon::duration::microseconds(1);
        tv = d.to_timeval();
        EXPECT_EQ(max_timeval_sec, tv.tv_sec);
        EXPECT_EQ(999999, tv.tv_usec);
        d += melon::duration::microseconds(1);  // no effect
        tv = d.to_timeval();
        EXPECT_EQ(max_timeval_sec, tv.tv_sec);
        EXPECT_EQ(999999, tv.tv_usec);

        tv.tv_sec = min_timeval_sec;
        tv.tv_usec = 1;
        d = melon::duration::from_timeval(tv);
        tv = d.to_timeval();
        EXPECT_EQ(min_timeval_sec, tv.tv_sec);
        EXPECT_EQ(1, tv.tv_usec);
        d -= melon::duration::microseconds(1);
        tv = d.to_timeval();
        EXPECT_EQ(min_timeval_sec, tv.tv_sec);
        EXPECT_EQ(0, tv.tv_usec);
        d -= melon::duration::microseconds(1);  // no effect
        tv = d.to_timeval();
        EXPECT_EQ(min_timeval_sec, tv.tv_sec);
        EXPECT_EQ(0, tv.tv_usec);

        const auto max_timespec_sec =
                std::numeric_limits<decltype(timespec::tv_sec)>::max();
        const auto min_timespec_sec =
                std::numeric_limits<decltype(timespec::tv_sec)>::min();
        timespec ts;
        ts.tv_sec = max_timespec_sec;
        ts.tv_nsec = 999999998;
        d = melon::duration::from_timespec(ts);
        ts = d.to_timespec();
        EXPECT_EQ(max_timespec_sec, ts.tv_sec);
        EXPECT_EQ(999999998, ts.tv_nsec);
        d += melon::duration::nanoseconds(1);
        ts = d.to_timespec();
        EXPECT_EQ(max_timespec_sec, ts.tv_sec);
        EXPECT_EQ(999999999, ts.tv_nsec);
        d += melon::duration::nanoseconds(1);  // no effect
        ts = d.to_timespec();
        EXPECT_EQ(max_timespec_sec, ts.tv_sec);
        EXPECT_EQ(999999999, ts.tv_nsec);

        ts.tv_sec = min_timespec_sec;
        ts.tv_nsec = 1;
        d = melon::duration::from_timespec(ts);
        ts =d.to_timespec();
        EXPECT_EQ(min_timespec_sec, ts.tv_sec);
        EXPECT_EQ(1, ts.tv_nsec);
        d -= melon::duration::nanoseconds(1);
        ts = d.to_timespec();
        EXPECT_EQ(min_timespec_sec, ts.tv_sec);
        EXPECT_EQ(0, ts.tv_nsec);
        d -= melon::duration::nanoseconds(1);  // no effect
        ts = d.to_timespec();
        EXPECT_EQ(min_timespec_sec, ts.tv_sec);
        EXPECT_EQ(0, ts.tv_nsec);
    }

    TEST(duration, format_duration) {
        // Example from Go's docs.
        EXPECT_EQ("72h3m0.5s",
                  (melon::duration::hours(72) + melon::duration::minutes(3) +
                   melon::duration::milliseconds(500)).format_duration());
        // Go's largest time: 2540400h10m10.000000000s
        EXPECT_EQ("2540400h10m10s",
                  (melon::duration::hours(2540400) + melon::duration::minutes(10) +
                   melon::duration::seconds(10)).format_duration());

        EXPECT_EQ("0", melon::zero_duration().format_duration());
        EXPECT_EQ("0", melon::duration::seconds(0).format_duration());
        EXPECT_EQ("0", melon::duration::nanoseconds(0).format_duration());

        EXPECT_EQ("1ns", melon::duration::nanoseconds(1).format_duration());
        EXPECT_EQ("1us", melon::duration::microseconds(1).format_duration());
        EXPECT_EQ("1ms", melon::duration::milliseconds(1).format_duration());
        EXPECT_EQ("1s", melon::duration::seconds(1).format_duration());
        EXPECT_EQ("1m", melon::duration::minutes(1).format_duration());
        EXPECT_EQ("1h", melon::duration::hours(1).format_duration());

        EXPECT_EQ("1h1m", (melon::duration::hours(1) + melon::duration::minutes(1)).format_duration());
        EXPECT_EQ("1h1s", (melon::duration::hours(1) + melon::duration::seconds(1)).format_duration());
        EXPECT_EQ("1m1s", (melon::duration::minutes(1) + melon::duration::seconds(1)).format_duration());

        EXPECT_EQ("1h0.25s",
                  (melon::duration::hours(1) + melon::duration::milliseconds(250)).format_duration());
        EXPECT_EQ("1m0.25s",
                  (melon::duration::minutes(1) + melon::duration::milliseconds(250)).format_duration());
        EXPECT_EQ("1h1m0.25s",
                  (melon::duration::hours(1) + melon::duration::minutes(1) +
                  melon::duration::milliseconds(250)).format_duration());
        EXPECT_EQ("1h0.0005s",
                  (melon::duration::hours(1) + melon::duration::microseconds(500)).format_duration());
        EXPECT_EQ("1h0.0000005s",
                  (melon::duration::hours(1) + melon::duration::nanoseconds(500)).format_duration());

        // Subsecond special case.
        EXPECT_EQ("1.5ns", (melon::duration::nanoseconds(1) +
                            melon::duration::nanoseconds(1) / 2).format_duration());
        EXPECT_EQ("1.25ns", (melon::duration::nanoseconds(1) +
                             melon::duration::nanoseconds(1) / 4).format_duration());
        EXPECT_EQ("1ns", (melon::duration::nanoseconds(1) +
                          melon::duration::nanoseconds(1) / 9).format_duration());
        EXPECT_EQ("1.2us", (melon::duration::microseconds(1) +
                           melon::duration::nanoseconds(200)).format_duration());
        EXPECT_EQ("1.2ms", (melon::duration::milliseconds(1) +
                            melon::duration::microseconds(200)).format_duration());
        EXPECT_EQ("1.0002ms", (melon::duration::milliseconds(1) +
                               melon::duration::nanoseconds(200)).format_duration());
        EXPECT_EQ("1.00001ms", (melon::duration::milliseconds(1) +
                               melon::duration::nanoseconds(10)).format_duration());
        EXPECT_EQ("1.000001ms",
                  (melon::duration::milliseconds(1) + melon::duration::nanoseconds(1)).format_duration());

        // Negative durations.
        EXPECT_EQ("-1ns", (melon::duration::nanoseconds(-1)).format_duration());
        EXPECT_EQ("-1us", (melon::duration::microseconds(-1)).format_duration());
        EXPECT_EQ("-1ms", (melon::duration::milliseconds(-1)).format_duration());
        EXPECT_EQ("-1s", (melon::duration::seconds(-1)).format_duration());
        EXPECT_EQ("-1m", (melon::duration::minutes(-1)).format_duration());
        EXPECT_EQ("-1h", (melon::duration::hours(-1)).format_duration());

        EXPECT_EQ("-1h1m",
                  (-(melon::duration::hours(1) + melon::duration::minutes(1))).format_duration());
        EXPECT_EQ("-1h1s",
                  (-(melon::duration::hours(1) + melon::duration::seconds(1))).format_duration());
        EXPECT_EQ("-1m1s",
                  (-(melon::duration::minutes(1) + melon::duration::seconds(1))).format_duration());

        EXPECT_EQ("-1ns", (melon::duration::nanoseconds(-1)).format_duration());
        EXPECT_EQ("-1.2us", (-(melon::duration::microseconds(1) + melon::duration::nanoseconds(200))).format_duration());
        EXPECT_EQ("-1.2ms", (-(melon::duration::milliseconds(1) + melon::duration::microseconds(200))).format_duration());
        EXPECT_EQ("-1.0002ms", (-(melon::duration::milliseconds(1) +
                                  melon::duration::nanoseconds(200))).format_duration());
        EXPECT_EQ("-1.00001ms", (-(melon::duration::milliseconds(1) +
                                   melon::duration::nanoseconds(10))).format_duration());
        EXPECT_EQ("-1.000001ms", (-(melon::duration::milliseconds(1) +
                                    melon::duration::nanoseconds(1))).format_duration());

        //
        // Interesting corner cases.
        //

        const melon::duration qns = melon::duration::nanoseconds(1) / 4;
        const melon::duration max_dur =
                melon::duration::seconds(kint64max) + (melon::duration::seconds(1) - qns);
        const melon::duration min_dur = melon::duration::seconds(kint64min);

        EXPECT_EQ("0.25ns", qns.format_duration());
        EXPECT_EQ("-0.25ns", (-qns).format_duration());
        EXPECT_EQ("2562047788015215h30m7.99999999975s",
                  max_dur.format_duration());
        EXPECT_EQ("-2562047788015215h30m8s", min_dur.format_duration());

        // Tests printing full precision from units that print using float_div_duration
        EXPECT_EQ("55.00000000025s", (melon::duration::seconds(55) + qns).format_duration());
        EXPECT_EQ("55.00000025ms",
                  (melon::duration::milliseconds(55) + qns).format_duration());
        EXPECT_EQ("55.00025us", (melon::duration::microseconds(55) + qns).format_duration());
        EXPECT_EQ("55.25ns", (melon::duration::nanoseconds(55) + qns).format_duration());

        // Formatting infinity
        EXPECT_EQ("inf", melon::infinite_duration().format_duration());
        EXPECT_EQ("-inf", (-melon::infinite_duration()).format_duration());

        // Formatting approximately +/- 100 billion years
        const melon::duration huge_range = ApproxYears(100000000000);
        EXPECT_EQ("876000000000000h", huge_range.format_duration());
        EXPECT_EQ("-876000000000000h", (-huge_range).format_duration());

        EXPECT_EQ("876000000000000h0.999999999s",
                  (huge_range +
                   (melon::duration::seconds(1) - melon::duration::nanoseconds(1))).format_duration());
        EXPECT_EQ("876000000000000h0.9999999995s",
                  (huge_range + (melon::duration::seconds(1) - melon::duration::nanoseconds(1) / 2)).format_duration());
        EXPECT_EQ("876000000000000h0.99999999975s",
                  (huge_range + (melon::duration::seconds(1) - melon::duration::nanoseconds(1) / 4)).format_duration());

        EXPECT_EQ("-876000000000000h0.999999999s",
                  (-huge_range -
                   (melon::duration::seconds(1) - melon::duration::nanoseconds(1))).format_duration());
        EXPECT_EQ("-876000000000000h0.9999999995s",
                  (-huge_range - (melon::duration::seconds(1) - melon::duration::nanoseconds(1) / 2)).format_duration());
        EXPECT_EQ("-876000000000000h0.99999999975s",
                  ( -huge_range - (melon::duration::seconds(1) - melon::duration::nanoseconds(1) / 4)).format_duration());
    }

    TEST(duration, parse_duration) {
        melon::duration d;

        // No specified unit. Should only work for zero and infinity.
        EXPECT_TRUE(melon::parse_duration("0", &d));
        EXPECT_EQ(melon::zero_duration(), d);
        EXPECT_TRUE(melon::parse_duration("+0", &d));
        EXPECT_EQ(melon::zero_duration(), d);
        EXPECT_TRUE(melon::parse_duration("-0", &d));
        EXPECT_EQ(melon::zero_duration(), d);

        EXPECT_TRUE(melon::parse_duration("inf", &d));
        EXPECT_EQ(melon::infinite_duration(), d);
        EXPECT_TRUE(melon::parse_duration("+inf", &d));
        EXPECT_EQ(melon::infinite_duration(), d);
        EXPECT_TRUE(melon::parse_duration("-inf", &d));
        EXPECT_EQ(-melon::infinite_duration(), d);
        EXPECT_FALSE(melon::parse_duration("infBlah", &d));

        // Illegal input forms.
        EXPECT_FALSE(melon::parse_duration("", &d));
        EXPECT_FALSE(melon::parse_duration("0.0", &d));
        EXPECT_FALSE(melon::parse_duration(".0", &d));
        EXPECT_FALSE(melon::parse_duration(".", &d));
        EXPECT_FALSE(melon::parse_duration("01", &d));
        EXPECT_FALSE(melon::parse_duration("1", &d));
        EXPECT_FALSE(melon::parse_duration("-1", &d));
        EXPECT_FALSE(melon::parse_duration("2", &d));
        EXPECT_FALSE(melon::parse_duration("2 s", &d));
        EXPECT_FALSE(melon::parse_duration(".s", &d));
        EXPECT_FALSE(melon::parse_duration("-.s", &d));
        EXPECT_FALSE(melon::parse_duration("s", &d));
        EXPECT_FALSE(melon::parse_duration(" 2s", &d));
        EXPECT_FALSE(melon::parse_duration("2s ", &d));
        EXPECT_FALSE(melon::parse_duration(" 2s ", &d));
        EXPECT_FALSE(melon::parse_duration("2mt", &d));
        EXPECT_FALSE(melon::parse_duration("1e3s", &d));

        // One unit type.
        EXPECT_TRUE(melon::parse_duration("1ns", &d));
        EXPECT_EQ(melon::duration::nanoseconds(1), d);
        EXPECT_TRUE(melon::parse_duration("1us", &d));
        EXPECT_EQ(melon::duration::microseconds(1), d);
        EXPECT_TRUE(melon::parse_duration("1ms", &d));
        EXPECT_EQ(melon::duration::milliseconds(1), d);
        EXPECT_TRUE(melon::parse_duration("1s", &d));
        EXPECT_EQ(melon::duration::seconds(1), d);
        EXPECT_TRUE(melon::parse_duration("2m", &d));
        EXPECT_EQ(melon::duration::minutes(2), d);
        EXPECT_TRUE(melon::parse_duration("2h", &d));
        EXPECT_EQ(melon::duration::hours(2), d);

        // Huge counts of a unit.
        EXPECT_TRUE(melon::parse_duration("9223372036854775807us", &d));
        EXPECT_EQ(melon::duration::microseconds(9223372036854775807), d);
        EXPECT_TRUE(melon::parse_duration("-9223372036854775807us", &d));
        EXPECT_EQ(melon::duration::microseconds(-9223372036854775807), d);

        // Multiple units.
        EXPECT_TRUE(melon::parse_duration("2h3m4s", &d));
        EXPECT_EQ(melon::duration::hours(2) + melon::duration::minutes(3) + melon::duration::seconds(4), d);
        EXPECT_TRUE(melon::parse_duration("3m4s5us", &d));
        EXPECT_EQ(melon::duration::minutes(3) + melon::duration::seconds(4) + melon::duration::microseconds(5), d);
        EXPECT_TRUE(melon::parse_duration("2h3m4s5ms6us7ns", &d));
        EXPECT_EQ(melon::duration::hours(2) + melon::duration::minutes(3) + melon::duration::seconds(4) +
                  melon::duration::milliseconds(5) + melon::duration::microseconds(6) +
                  melon::duration::nanoseconds(7),
                  d);

        // Multiple units out of order.
        EXPECT_TRUE(melon::parse_duration("2us3m4s5h", &d));
        EXPECT_EQ(melon::duration::hours(5) + melon::duration::minutes(3) + melon::duration::seconds(4) +
                  melon::duration::microseconds(2),
                  d);

        // Fractional values of units.
        EXPECT_TRUE(melon::parse_duration("1.5ns", &d));
        EXPECT_EQ(1.5 * melon::duration::nanoseconds(1), d);
        EXPECT_TRUE(melon::parse_duration("1.5us", &d));
        EXPECT_EQ(1.5 * melon::duration::microseconds(1), d);
        EXPECT_TRUE(melon::parse_duration("1.5ms", &d));
        EXPECT_EQ(1.5 * melon::duration::milliseconds(1), d);
        EXPECT_TRUE(melon::parse_duration("1.5s", &d));
        EXPECT_EQ(1.5 * melon::duration::seconds(1), d);
        EXPECT_TRUE(melon::parse_duration("1.5m", &d));
        EXPECT_EQ(1.5 * melon::duration::minutes(1), d);
        EXPECT_TRUE(melon::parse_duration("1.5h", &d));
        EXPECT_EQ(1.5 * melon::duration::hours(1), d);

        // Huge fractional counts of a unit.
        EXPECT_TRUE(melon::parse_duration("0.4294967295s", &d));
        EXPECT_EQ(melon::duration::nanoseconds(429496729) + melon::duration::nanoseconds(1) / 2, d);
        EXPECT_TRUE(melon::parse_duration("0.429496729501234567890123456789s", &d));
        EXPECT_EQ(melon::duration::nanoseconds(429496729) + melon::duration::nanoseconds(1) / 2, d);

        // Negative durations.
        EXPECT_TRUE(melon::parse_duration("-1s", &d));
        EXPECT_EQ(melon::duration::seconds(-1), d);
        EXPECT_TRUE(melon::parse_duration("-1m", &d));
        EXPECT_EQ(melon::duration::minutes(-1), d);
        EXPECT_TRUE(melon::parse_duration("-1h", &d));
        EXPECT_EQ(melon::duration::hours(-1), d);

        EXPECT_TRUE(melon::parse_duration("-1h2s", &d));
        EXPECT_EQ(-(melon::duration::hours(1) + melon::duration::seconds(2)), d);
        EXPECT_FALSE(melon::parse_duration("1h-2s", &d));
        EXPECT_FALSE(melon::parse_duration("-1h-2s", &d));
        EXPECT_FALSE(melon::parse_duration("-1h -2s", &d));
    }

    TEST(duration, FormatParseRoundTrip) {
#define TEST_PARSE_ROUNDTRIP(d)                \
  do {                                         \
    std::string s = (d).format_duration();   \
    melon::duration dur;                        \
    EXPECT_TRUE(melon::parse_duration(s, &dur)); \
    EXPECT_EQ(d, dur);                         \
  } while (0)

        TEST_PARSE_ROUNDTRIP(melon::duration::nanoseconds(1));
        TEST_PARSE_ROUNDTRIP(melon::duration::microseconds(1));
        TEST_PARSE_ROUNDTRIP(melon::duration::milliseconds(1));
        TEST_PARSE_ROUNDTRIP(melon::duration::seconds(1));
        TEST_PARSE_ROUNDTRIP(melon::duration::minutes(1));
        TEST_PARSE_ROUNDTRIP(melon::duration::hours(1));
        TEST_PARSE_ROUNDTRIP(melon::duration::hours(1) + melon::duration::nanoseconds(2));

        TEST_PARSE_ROUNDTRIP(melon::duration::nanoseconds(-1));
        TEST_PARSE_ROUNDTRIP(melon::duration::microseconds(-1));
        TEST_PARSE_ROUNDTRIP(melon::duration::milliseconds(-1));
        TEST_PARSE_ROUNDTRIP(melon::duration::seconds(-1));
        TEST_PARSE_ROUNDTRIP(melon::duration::minutes(-1));
        TEST_PARSE_ROUNDTRIP(melon::duration::hours(-1));

        TEST_PARSE_ROUNDTRIP(melon::duration::hours(-1) + melon::duration::nanoseconds(2));
        TEST_PARSE_ROUNDTRIP(melon::duration::hours(1) + melon::duration::nanoseconds(-2));
        TEST_PARSE_ROUNDTRIP(melon::duration::hours(-1) + melon::duration::nanoseconds(-2));

        TEST_PARSE_ROUNDTRIP(melon::duration::nanoseconds(1) +
                             melon::duration::nanoseconds(1) / 4);  // 1.25ns

        const melon::duration huge_range = ApproxYears(100000000000);
        TEST_PARSE_ROUNDTRIP(huge_range);
        TEST_PARSE_ROUNDTRIP(huge_range + (melon::duration::seconds(1) - melon::duration::nanoseconds(1)));

#undef TEST_PARSE_ROUNDTRIP
    }

}  // namespace

