//
// Created by liyinbin on 2020/1/29.
//

#ifndef ABEL_CHRONO_TIME_H_
#define ABEL_CHRONO_TIME_H_

#if !defined(_MSC_VER)
#include <sys/time.h>
#else
// We don't include `winsock2.h` because it drags in `windows.h` and friends,
// and they define conflicting macros like OPAQUE, ERROR, and more. This has the
// potential to break abel users.
//
// Instead we only forward declare `timeval` and require Windows users include
// `winsock2.h` themselves. This is both inconsistent and troublesome, but so is
// including 'windows.h' so we are picking the lesser of two evils here.
struct timeval;
#endif
#include <chrono>  // NOLINT(build/c++11)
#include <cmath>
#include <cstdint>
#include <ctime>
#include <ostream>
#include <string>
#include <type_traits>
#include <utility>
#include <abel/base/profile.h>
#include <abel/strings/string_view.h>
#include <abel/chrono/internal/chrono_time_internal.h>
#include <abel/chrono/civil_time.h>
#include <abel/chrono/internal/time_zone.h>

namespace abel {

class duration;  // Defined below
class abel_time;      // Defined below
class time_zone;  // Defined below

namespace chrono_internal {
int64_t integer_div_duration (bool satq, duration num, duration den, duration *rem);
constexpr abel_time from_unix_duration (duration d);
constexpr duration to_unix_duration (abel_time t);
constexpr int64_t get_rep_hi (duration d);
constexpr uint32_t get_rep_lo (duration d);
constexpr duration make_duration (int64_t hi, uint32_t lo);
constexpr duration make_duration (int64_t hi, int64_t lo);
ABEL_FORCE_INLINE duration make_pos_double_duration (double n);
constexpr int64_t kTicksPerNanosecond = 4;
constexpr int64_t kTicksPerSecond = 1000 * 1000 * 1000 * kTicksPerNanosecond;
template<std::intmax_t N>
constexpr duration from_int64 (int64_t v, std::ratio<1, N>);
constexpr duration from_int64 (int64_t v, std::ratio<60>);
constexpr duration from_int64 (int64_t v, std::ratio<3600>);
template<typename T>
using EnableIfIntegral = typename std::enable_if<
    std::is_integral<T>::value || std::is_enum<T>::value, int>::type;
template<typename T>
using EnableIfFloat =
typename std::enable_if<std::is_floating_point<T>::value, int>::type;
}  // namespace chrono_internal


// duration
//
// The `abel::duration` class represents a signed, fixed-length span of time.
// A `duration` is generated using a unit-specific factory function, or is
// the result of subtracting one `abel::abel_time` from another. Durations behave
// like unit-safe integers and they support all the natural integer-like
// arithmetic operations. Arithmetic overflows and saturates at +/- infinity.
// `duration` should be passed by value rather than const reference.
//
// Factory functions `nanoseconds()`, `microseconds()`, `milliseconds()`,
// `seconds()`, `minutes()`, `hours()` and `infinite_duration()` allow for
// creation of constexpr `duration` values
//
// Examples:
//
//   constexpr abel::duration ten_ns = abel::nanoseconds(10);
//   constexpr abel::duration min = abel::minutes(1);
//   constexpr abel::duration hour = abel::hours(1);
//   abel::duration dur = 60 * min;  // dur == hour
//   abel::duration half_sec = abel::milliseconds(500);
//   abel::duration quarter_sec = 0.25 * abel::seconds(1);
//
// `duration` values can be easily converted to an integral number of units
// using the division operator.
//
// Example:
//
//   constexpr abel::duration dur = abel::milliseconds(1500);
//   int64_t ns = dur / abel::nanoseconds(1);   // ns == 1500000000
//   int64_t ms = dur / abel::milliseconds(1);  // ms == 1500
//   int64_t sec = dur / abel::seconds(1);    // sec == 1 (subseconds truncated)
//   int64_t min = dur / abel::minutes(1);    // min == 0
//
// See the `integer_div_duration()` and `float_div_duration()` functions below for details on
// how to access the fractional parts of the quotient.
//
// Alternatively, conversions can be performed using helpers such as
// `to_int64_microseconds()` and `to_double_seconds()`.
class duration {
public:
    // Value semantics.
    constexpr duration () : rep_hi_(0), rep_lo_(0) { }  // zero-length duration

    // Copyable.
#if !defined(__clang__) && defined(_MSC_VER) && _MSC_VER < 1910
    // Explicitly defining the constexpr copy constructor avoids an MSVC bug.
    constexpr duration(const duration& d)
        : rep_hi_(d.rep_hi_), rep_lo_(d.rep_lo_) {}
#else
    constexpr duration (const duration &d) = default;
#endif
    duration &operator = (const duration &d) = default;

    // Compound assignment operators.
    duration &operator += (duration d);
    duration &operator -= (duration d);
    duration &operator *= (int64_t r);
    duration &operator *= (double r);
    duration &operator /= (int64_t r);
    duration &operator /= (double r);
    duration &operator %= (duration rhs);

    // Overloads that forward to either the int64_t or double overloads above.
    // Integer operands must be representable as int64_t.
    template<typename T>
    duration &operator *= (T r) {
        int64_t x = r;
        return *this *= x;
    }
    template<typename T>
    duration &operator /= (T r) {
        int64_t x = r;
        return *this /= x;
    }
    duration &operator *= (float r) { return *this *= static_cast<double>(r); }
    duration &operator /= (float r) { return *this /= static_cast<double>(r); }

    template<typename H>
    friend H AbelHashValue (H h, duration d) {
        return H::combine(std::move(h), d.rep_hi_, d.rep_lo_);
    }

private:
    friend constexpr int64_t chrono_internal::get_rep_hi (duration d);
    friend constexpr uint32_t chrono_internal::get_rep_lo (duration d);
    friend constexpr duration chrono_internal::make_duration (int64_t hi,
                                                             uint32_t lo);
    constexpr duration (int64_t hi, uint32_t lo) : rep_hi_(hi), rep_lo_(lo) { }
    int64_t rep_hi_;
    uint32_t rep_lo_;
};

// Relational Operators
constexpr bool operator < (duration lhs, duration rhs);
constexpr bool operator > (duration lhs, duration rhs) { return rhs < lhs; }
constexpr bool operator >= (duration lhs, duration rhs) { return !(lhs < rhs); }
constexpr bool operator <= (duration lhs, duration rhs) { return !(rhs < lhs); }
constexpr bool operator == (duration lhs, duration rhs);
constexpr bool operator != (duration lhs, duration rhs) { return !(lhs == rhs); }

// Additive Operators
constexpr duration operator - (duration d);
ABEL_FORCE_INLINE duration operator + (duration lhs, duration rhs) { return lhs += rhs; }
ABEL_FORCE_INLINE duration operator - (duration lhs, duration rhs) { return lhs -= rhs; }

// Multiplicative Operators
// Integer operands must be representable as int64_t.
template<typename T>
duration operator * (duration lhs, T rhs) {
    return lhs *= rhs;
}
template<typename T>
duration operator * (T lhs, duration rhs) {
    return rhs *= lhs;
}
template<typename T>
duration operator / (duration lhs, T rhs) {
    return lhs /= rhs;
}
ABEL_FORCE_INLINE int64_t operator / (duration lhs, duration rhs) {
    return chrono_internal::integer_div_duration(true, lhs, rhs,
                                                 &lhs);  // trunc towards zero
}
ABEL_FORCE_INLINE duration operator % (duration lhs, duration rhs) { return lhs %= rhs; }

// integer_div_duration()
//
// Divides a numerator `duration` by a denominator `duration`, returning the
// quotient and remainder. The remainder always has the same sign as the
// numerator. The returned quotient and remainder respect the identity:
//
//   numerator = denominator * quotient + remainder
//
// Returned quotients are capped to the range of `int64_t`, with the difference
// spilling into the remainder to uphold the above identity. This means that the
// remainder returned could differ from the remainder returned by
// `duration::operator%` for huge quotients.
//
// See also the notes on `infinite_duration()` below regarding the behavior of
// division involving zero and infinite durations.
//
// Example:
//
//   constexpr abel::duration a =
//       abel::seconds(std::numeric_limits<int64_t>::max());  // big
//   constexpr abel::duration b = abel::nanoseconds(1);       // small
//
//   abel::duration rem = a % b;
//   // rem == abel::zero_duration()
//
//   // Here, q would overflow int64_t, so rem accounts for the difference.
//   int64_t q = abel::integer_div_duration(a, b, &rem);
//   // q == std::numeric_limits<int64_t>::max(), rem == a - b * q
ABEL_FORCE_INLINE int64_t integer_div_duration (duration num, duration den, duration *rem) {
    return chrono_internal::integer_div_duration(true, num, den,
                                                 rem);  // trunc towards zero
}

// float_div_duration()
//
// Divides a `duration` numerator into a fractional number of units of a
// `duration` denominator.
//
// See also the notes on `infinite_duration()` below regarding the behavior of
// division involving zero and infinite durations.
//
// Example:
//
//   double d = abel::float_div_duration(abel::milliseconds(1500), abel::seconds(1));
//   // d == 1.5
double float_div_duration (duration num, duration den);

// zero_duration()
//
// Returns a zero-length duration. This function behaves just like the default
// constructor, but the name helps make the semantics clear at call sites.
constexpr duration zero_duration () { return duration(); }

// abs_duration()
//
// Returns the absolute value of a duration.
ABEL_FORCE_INLINE duration abs_duration (duration d) {
    return (d < zero_duration()) ? -d : d;
}

// trunc()
//
// Truncates a duration (toward zero) to a multiple of a non-zero unit.
//
// Example:
//
//   abel::duration d = abel::nanoseconds(123456789);
//   abel::duration a = abel::trunc(d, abel::microseconds(1));  // 123456us
duration trunc (duration d, duration unit);

// floor()
//
// Floors a duration using the passed duration unit to its largest value not
// greater than the duration.
//
// Example:
//
//   abel::duration d = abel::nanoseconds(123456789);
//   abel::duration b = abel::floor(d, abel::microseconds(1));  // 123456us
duration floor (duration d, duration unit);

// ceil()
//
// Returns the ceiling of a duration using the passed duration unit to its
// smallest value not less than the duration.
//
// Example:
//
//   abel::duration d = abel::nanoseconds(123456789);
//   abel::duration c = abel::ceil(d, abel::microseconds(1));   // 123457us
duration ceil (duration d, duration unit);

// infinite_duration()
//
// Returns an infinite `duration`.  To get a `duration` representing negative
// infinity, use `-infinite_duration()`.
//
// duration arithmetic overflows to +/- infinity and saturates. In general,
// arithmetic with `duration` infinities is similar to IEEE 754 infinities
// except where IEEE 754 NaN would be involved, in which case +/-
// `infinite_duration()` is used in place of a "nan" duration.
//
// Examples:
//
//   constexpr abel::duration inf = abel::infinite_duration();
//   const abel::duration d = ... any finite duration ...
//
//   inf == inf + inf
//   inf == inf + d
//   inf == inf - inf
//   -inf == d - inf
//
//   inf == d * 1e100
//   inf == inf / 2
//   0 == d / inf
//   INT64_MAX == inf / d
//
//   d < inf
//   -inf < d
//
//   // Division by zero returns infinity, or INT64_MIN/MAX where appropriate.
//   inf == d / 0
//   INT64_MAX == d / abel::zero_duration()
//
// The examples involving the `/` operator above also apply to `integer_div_duration()`
// and `float_div_duration()`.
constexpr duration infinite_duration ();

// nanoseconds()
// microseconds()
// milliseconds()
// seconds()
// minutes()
// hours()
//
// Factory functions for constructing `duration` values from an integral number
// of the unit indicated by the factory function's name. The number must be
// representable as int64_t.
//
// NOTE: no "Days()" factory function exists because "a day" is ambiguous.
// Civil days are not always 24 hours long, and a 24-hour duration often does
// not correspond with a civil day. If a 24-hour duration is needed, use
// `abel::hours(24)`. If you actually want a civil day, use abel::chrono_day
// from civil_time.h.
//
// Example:
//
//   abel::duration a = abel::seconds(60);
//   abel::duration b = abel::minutes(1);  // b == a
constexpr duration nanoseconds (int64_t n);
constexpr duration microseconds (int64_t n);
constexpr duration milliseconds (int64_t n);
constexpr duration seconds (int64_t n);
constexpr duration minutes (int64_t n);
constexpr duration hours (int64_t n);

// Factory overloads for constructing `duration` values from a floating-point
// number of the unit indicated by the factory function's name. These functions
// exist for convenience, but they are not as efficient as the integral
// factories, which should be preferred.
//
// Example:
//
//   auto a = abel::seconds(1.5);        // OK
//   auto b = abel::milliseconds(1500);  // BETTER
template<typename T, chrono_internal::EnableIfFloat<T> = 0>
duration nanoseconds (T n) {
    return n * nanoseconds(1);
}
template<typename T, chrono_internal::EnableIfFloat<T> = 0>
duration microseconds (T n) {
    return n * microseconds(1);
}
template<typename T, chrono_internal::EnableIfFloat<T> = 0>
duration milliseconds (T n) {
    return n * milliseconds(1);
}
template<typename T, chrono_internal::EnableIfFloat<T> = 0>
duration seconds (T n) {
    if (n >= 0) {  // Note: `NaN >= 0` is false.
        if (n >= static_cast<T>((std::numeric_limits<int64_t>::max)())) {
            return infinite_duration();
        }
        return chrono_internal::make_pos_double_duration(n);
    } else {
        if (std::isnan(n))
            return std::signbit(n) ? -infinite_duration() : infinite_duration();
        if (n <= (std::numeric_limits<int64_t>::min)())
            return -infinite_duration();
        return -chrono_internal::make_pos_double_duration(-n);
    }
}
template<typename T, chrono_internal::EnableIfFloat<T> = 0>
duration minutes (T n) {
    return n * minutes(1);
}
template<typename T, chrono_internal::EnableIfFloat<T> = 0>
duration hours (T n) {
    return n * hours(1);
}

// to_int64_nanoseconds()
// to_int64_microseconds()
// to_int64_milliseconds()
// to_int64_seconds()
// to_int64_minutes()
// to_int64_hours()
//
// Helper functions that convert a duration to an integral count of the
// indicated unit. These functions are shorthand for the `integer_div_duration()`
// function above; see its documentation for details about overflow, etc.
//
// Example:
//
//   abel::duration d = abel::milliseconds(1500);
//   int64_t isec = abel::to_int64_seconds(d);  // isec == 1
int64_t to_int64_nanoseconds (duration d);
int64_t to_int64_microseconds (duration d);
int64_t to_int64_milliseconds (duration d);
int64_t to_int64_seconds (duration d);
int64_t to_int64_minutes (duration d);
int64_t to_int64_hours (duration d);

//
// Helper functions that convert a duration to a floating point count of the
// indicated unit. These functions are shorthand for the `float_div_duration()`
// function above; see its documentation for details about overflow, etc.
//
// Example:
//
//   abel::duration d = abel::milliseconds(1500);
//   double dsec = abel::ToDoubleSeconds(d);  // dsec == 1.5
double to_double_nanoseconds (duration d);
double to_double_microseconds (duration d);
double to_double_milliseconds (duration d);
double to_double_seconds (duration d);
double to_double_minutes (duration d);
double to_double_hours (duration d);

// from_chrono()
//
// Converts any of the pre-defined std::chrono durations to an abel::duration.
//
// Example:
//
//   std::chrono::milliseconds ms(123);
//   abel::duration d = abel::from_chrono(ms);
constexpr duration from_chrono (const std::chrono::nanoseconds &d);
constexpr duration from_chrono (const std::chrono::microseconds &d);
constexpr duration from_chrono (const std::chrono::milliseconds &d);
constexpr duration from_chrono (const std::chrono::seconds &d);
constexpr duration from_chrono (const std::chrono::minutes &d);
constexpr duration from_chrono (const std::chrono::hours &d);

// to_chrono_nanoseconds()
// to_chrono_microseconds()
// to_chrono_milliseconds()
// to_chrono_seconds()
// to_chrono_minutes()
// to_chrono_hours()
//
// Converts an abel::duration to any of the pre-defined std::chrono durations.
// If overflow would occur, the returned value will saturate at the min/max
// chrono duration value instead.
//
// Example:
//
//   abel::duration d = abel::microseconds(123);
//   auto x = abel::to_chrono_microseconds(d);
//   auto y = abel::to_chrono_nanoseconds(d);  // x == y
//   auto z = abel::to_chrono_seconds(abel::infinite_duration());
//   // z == std::chrono::seconds::max()
std::chrono::nanoseconds to_chrono_nanoseconds (duration d);
std::chrono::microseconds to_chrono_microseconds (duration d);
std::chrono::milliseconds to_chrono_milliseconds (duration d);
std::chrono::seconds to_chrono_seconds (duration d);
std::chrono::minutes to_chrono_minutes (duration d);
std::chrono::hours to_chrono_hours (duration d);

// format_duration()
//
// Returns a string representing the duration in the form "72h3m0.5s".
// Returns "inf" or "-inf" for +/- `infinite_duration()`.
std::string format_duration (duration d);

// Output stream operator.
ABEL_FORCE_INLINE std::ostream &operator << (std::ostream &os, duration d) {
    return os << format_duration(d);
}

// parse_duration()
//
// Parses a duration string consisting of a possibly signed sequence of
// decimal numbers, each with an optional fractional part and a unit
// suffix.  The valid suffixes are "ns", "us" "ms", "s", "m", and "h".
// Simple examples include "300ms", "-1.5h", and "2h45m".  Parses "0" as
// `zero_duration()`. Parses "inf" and "-inf" as +/- `infinite_duration()`.
bool parse_duration (const std::string &dur_string, duration *d);

// Support for flag values of type duration. duration flags must be specified
// in a format that is valid input for abel::parse_duration().
bool abel_parse_flag (abel::string_view text, duration *dst, std::string *error);
std::string abel_unparse_flag (duration d);
/*
ABEL_DEPRECATED_MESSAGE("Use abel_parse_flag() instead.")
bool ParseFlag(const std::string& text, duration* dst, std::string* error);
ABEL_DEPRECATED_MESSAGE("Use abel_unparse_flag() instead.")
std::string UnparseFlag(duration d);
*/
// abel_time
//
// An `abel::abel_time` represents a specific instant in time. Arithmetic operators
// are provided for naturally expressing time calculations. Instances are
// created using `abel::now()` and the `abel::From*()` factory functions that
// accept the gamut of other time representations. Formatting and parsing
// functions are provided for conversion to and from strings.  `abel::abel_time`
// should be passed by value rather than const reference.
//
// `abel::abel_time` assumes there are 60 seconds in a minute, which means the
// underlying time scales must be "smeared" to eliminate leap seconds.
// See https://developers.google.com/time/smear.
//
// Even though `abel::abel_time` supports a wide range of timestamps, exercise
// caution when using values in the distant past. `abel::abel_time` uses the
// Proleptic Gregorian calendar, which extends the Gregorian calendar backward
// to dates before its introduction in 1582.
// See https://en.wikipedia.org/wiki/Proleptic_Gregorian_calendar
// for more information. Use the ICU calendar classes to convert a date in
// some other calendar (http://userguide.icu-project.org/datetime/calendar).
//
// Similarly, standardized time zones are a reasonably recent innovation, with
// the Greenwich prime meridian being established in 1884. The TZ database
// itself does not profess accurate offsets for timestamps prior to 1970. The
// breakdown of future timestamps is subject to the whim of regional
// governments.
//
// The `abel::abel_time` class represents an instant in time as a count of clock
// ticks of some granularity (resolution) from some starting point (epoch).
//
// `abel::abel_time` uses a resolution that is high enough to avoid loss in
// precision, and a range that is wide enough to avoid overflow, when
// converting between tick counts in most Google time scales (i.e., resolution
// of at least one nanosecond, and range +/-100 billion years).  Conversions
// between the time scales are performed by truncating (towards negative
// infinity) to the nearest representable point.
//
// Examples:
//
//   abel::abel_time t1 = ...;
//   abel::abel_time t2 = t1 + abel::minutes(2);
//   abel::duration d = t2 - t1;  // == abel::minutes(2)
//
class abel_time {
public:
    // Value semantics.

    // Returns the Unix epoch.  However, those reading your code may not know
    // or expect the Unix epoch as the default value, so make your code more
    // readable by explicitly initializing all instances before use.
    //
    // Example:
    //   abel::abel_time t = abel::unix_epoch();
    //   abel::abel_time t = abel::now();
    //   abel::abel_time t = abel::time_from_timeval(tv);
    //   abel::abel_time t = abel::infinite_past();
    constexpr abel_time () = default;

    // Copyable.
    constexpr abel_time (const abel_time &t) = default;
    abel_time &operator = (const abel_time &t) = default;

    // Assignment operators.
    abel_time &operator += (duration d) {
        rep_ += d;
        return *this;
    }
    abel_time &operator -= (duration d) {
        rep_ -= d;
        return *this;
    }

    // abel_time::breakdown
    //
    // The calendar and wall-clock (aka "civil time") components of an
    // `abel::abel_time` in a certain `abel::time_zone`. This struct is not
    // intended to represent an instant in time. So, rather than passing
    // a `abel_time::breakdown` to a function, pass an `abel::abel_time` and an
    // `abel::time_zone`.
    //
    // Deprecated. Use `abel::time_zone::chrono_info`.
    struct breakdown {
        int64_t year;          // year (e.g., 2013)
        int month;           // month of year [1:12]
        int day;             // day of month [1:31]
        int hour;            // hour of day [0:23]
        int minute;          // minute of hour [0:59]
        int second;          // second of minute [0:59]
        duration subsecond;  // [seconds(0):seconds(1)) if finite
        int weekday;         // 1==Mon, ..., 7=Sun
        int yearday;         // day of year [1:366]

        // Note: The following fields exist for backward compatibility
        // with older APIs.  Accessing these fields directly is a sign of
        // imprudent logic in the calling code.  Modern time-related code
        // should only access this data indirectly by way of format_time().
        // These fields are undefined for infinite_future() and infinite_past().
        int offset;             // seconds east of UTC
        bool is_dst;            // is offset non-standard?
        const char *zone_abbr;  // time-zone abbreviation (e.g., "PST")
    };

    // abel_time::in()
    //
    // Returns the breakdown of this instant in the given time_zone.
    //
    // Deprecated. Use `abel::time_zone::At(abel_time)`.
    breakdown in (time_zone tz) const;

    template<typename H>
    friend H AbelHashValue (H h, abel_time t) {
        return H::combine(std::move(h), t.rep_);
    }

private:
    friend constexpr abel_time chrono_internal::from_unix_duration (duration d);
    friend constexpr duration chrono_internal::to_unix_duration (abel_time t);
    friend constexpr bool operator < (abel_time lhs, abel_time rhs);
    friend constexpr bool operator == (abel_time lhs, abel_time rhs);
    friend duration operator - (abel_time lhs, abel_time rhs);
    friend constexpr abel_time universal_epoch ();
    friend constexpr abel_time infinite_future ();
    friend constexpr abel_time infinite_past ();
    constexpr explicit abel_time (duration rep) : rep_(rep) { }
    duration rep_;
};

// Relational Operators
constexpr bool operator < (abel_time lhs, abel_time rhs) { return lhs.rep_ < rhs.rep_; }
constexpr bool operator > (abel_time lhs, abel_time rhs) { return rhs < lhs; }
constexpr bool operator >= (abel_time lhs, abel_time rhs) { return !(lhs < rhs); }
constexpr bool operator <= (abel_time lhs, abel_time rhs) { return !(rhs < lhs); }
constexpr bool operator == (abel_time lhs, abel_time rhs) { return lhs.rep_ == rhs.rep_; }
constexpr bool operator != (abel_time lhs, abel_time rhs) { return !(lhs == rhs); }

// Additive Operators
ABEL_FORCE_INLINE abel_time operator + (abel_time lhs, duration rhs) { return lhs += rhs; }
ABEL_FORCE_INLINE abel_time operator + (duration lhs, abel_time rhs) { return rhs += lhs; }
ABEL_FORCE_INLINE abel_time operator - (abel_time lhs, duration rhs) { return lhs -= rhs; }
ABEL_FORCE_INLINE duration operator - (abel_time lhs, abel_time rhs) { return lhs.rep_ - rhs.rep_; }

// unix_epoch()
//
// Returns the `abel::abel_time` representing "1970-01-01 00:00:00.0 +0000".
constexpr abel_time unix_epoch () { return abel_time(); }

// universal_epoch()
//
// Returns the `abel::abel_time` representing "0001-01-01 00:00:00.0 +0000", the
// epoch of the ICU Universal abel_time Scale.
constexpr abel_time universal_epoch () {
    // 719162 is the number of days from 0001-01-01 to 1970-01-01,
    // assuming the Gregorian calendar.
    return abel_time(chrono_internal::make_duration(-24 * 719162 * int64_t {3600}, 0U));
}

// infinite_future()
//
// Returns an `abel::abel_time` that is infinitely far in the future.
constexpr abel_time infinite_future () {
    return abel_time(
        chrono_internal::make_duration((std::numeric_limits<int64_t>::max)(), ~0U));
}

// infinite_past()
//
// Returns an `abel::abel_time` that is infinitely far in the past.
constexpr abel_time infinite_past () {
    return abel_time(
        chrono_internal::make_duration((std::numeric_limits<int64_t>::min)(), ~0U));
}

// from_unix_nanos()
// from_unix_micros()
// from_unix_millis()
// from_unix_seconds()
// from_time_t()
// from_date()
// from_universal()
//
// Creates an `abel::abel_time` from a variety of other representations.
constexpr abel_time from_unix_nanos (int64_t ns);
constexpr abel_time from_unix_micros (int64_t us);
constexpr abel_time from_unix_millis (int64_t ms);
constexpr abel_time from_unix_seconds (int64_t s);
constexpr abel_time from_time_t (time_t t);
abel_time from_date (double udate);
abel_time from_universal (int64_t universal);

// to_unix_nanos()
// to_unix_micros()
// to_unix_millis()
// to_unix_seconds()
// to_time_t()
// to_date()
// to_universal()
//
// Converts an `abel::abel_time` to a variety of other representations.  Note that
// these operations round down toward negative infinity where necessary to
// adjust to the resolution of the result type.  Beware of possible time_t
// over/underflow in ToTime{T,val,spec}() on 32-bit platforms.
int64_t to_unix_nanos (abel_time t);
int64_t to_unix_micros (abel_time t);
int64_t to_unix_millis (abel_time t);
int64_t to_unix_seconds (abel_time t);
time_t to_time_t (abel_time t);
double to_date (abel_time t);
int64_t to_universal (abel_time t);

// duration_from_timespec()
// duration_from_timeval()
// to_timespec()
// to_timeval()
// time_from_timespec()
// time_from_timeval()
// to_timespec()
// to_timeval()
//
// Some APIs use a timespec or a timeval as a duration (e.g., nanosleep(2)
// and select(2)), while others use them as a abel_time (e.g. clock_gettime(2)
// and gettimeofday(2)), so conversion functions are provided for both cases.
// The "to timespec/val" direction is easily handled via overloading, but
// for "from timespec/val" the desired type is part of the function name.
duration duration_from_timespec (timespec ts);
duration duration_from_timeval (timeval tv);
timespec to_timespec (duration d);
timeval to_timeval (duration d);
abel_time time_from_timespec (timespec ts);
abel_time time_from_timeval (timeval tv);
timespec to_timespec (abel_time t);
timeval to_timeval (abel_time t);

ABEL_FORCE_INLINE duration to_duration(abel_time t) {
    return nanoseconds(to_unix_nanos(t));
}

// from_chrono()
//
// Converts a std::chrono::system_clock::time_point to an abel::abel_time.
//
// Example:
//
//   auto tp = std::chrono::system_clock::from_time_t(123);
//   abel::abel_time t = abel::from_chrono(tp);
//   // t == abel::from_time_t(123)
abel_time from_chrono (const std::chrono::system_clock::time_point &tp);

// to_chrono_time()
//
// Converts an abel::abel_time to a std::chrono::system_clock::time_point. If
// overflow would occur, the returned value will saturate at the min/max time
// point value instead.
//
// Example:
//
//   abel::abel_time t = abel::from_time_t(123);
//   auto tp = abel::to_chrono_time(t);
//   // tp == std::chrono::system_clock::from_time_t(123);
std::chrono::system_clock::time_point to_chrono_time (abel_time);

// Support for flag values of type abel_time. abel_time flags must be specified in a
// format that matches abel::RFC3339_full. For example:
//
//   --start_time=2016-01-02T03:04:05.678+08:00
//
// Note: A UTC offset (or 'Z' indicating a zero-offset from UTC) is required.
//
// Additionally, if you'd like to specify a time as a count of
// seconds/milliseconds/etc from the Unix epoch, use an abel::duration flag
// and add that duration to abel::unix_epoch() to get an abel::abel_time.
bool abel_parse_flag (abel::string_view text, abel_time *t, std::string *error);
std::string abel_unparse_flag (abel_time t);
/*
ABEL_DEPRECATED_MESSAGE("Use abel_parse_flag() instead.")
bool ParseFlag(const std::string& text, abel_time* t, std::string* error);
ABEL_DEPRECATED_MESSAGE("Use abel_unparse_flag() instead.")
std::string UnparseFlag(abel_time t);
*/
// time_zone
//
// The `abel::time_zone` is an opaque, small, value-type class representing a
// geo-political region within which particular rules are used for converting
// between absolute and civil times (see https://git.io/v59Ly). `abel::time_zone`
// values are named using the TZ identifiers from the IANA abel_time Zone Database,
// such as "America/Los_Angeles" or "Australia/Sydney". `abel::time_zone` values
// are created from factory functions such as `abel::load_time_zone()`. Note:
// strings like "PST" and "EDT" are not valid TZ identifiers. Prefer to pass by
// value rather than const reference.
//
//
// Examples:
//
//   abel::time_zone utc = abel::utc_time_zone();
//   abel::time_zone pst = abel::fixed_time_zone(-8 * 60 * 60);
//   abel::time_zone loc = abel::local_time_zone();
//   abel::time_zone lax;
//   if (!abel::load_time_zone("America/Los_Angeles", &lax)) {
//     // handle error case
//   }
//
class time_zone {
public:
    explicit time_zone (abel::chrono_internal::time_zone tz) : cz_(tz) { }
    time_zone () = default;  // UTC, but prefer utc_time_zone() to be explicit.

    // Copyable.
    time_zone (const time_zone &) = default;
    time_zone &operator = (const time_zone &) = default;

    explicit operator abel::chrono_internal::time_zone () const { return cz_; }

    std::string name () const { return cz_.name(); }

    // time_zone::chrono_info
    //
    // Information about the civil time corresponding to an absolute time.
    // This struct is not intended to represent an instant in time. So, rather
    // than passing a `time_zone::chrono_info` to a function, pass an `abel::abel_time`
    // and an `abel::time_zone`.
    struct chrono_info {
        chrono_second cs;
        duration subsecond;

        // Note: The following fields exist for backward compatibility
        // with older APIs.  Accessing these fields directly is a sign of
        // imprudent logic in the calling code.  Modern time-related code
        // should only access this data indirectly by way of format_time().
        // These fields are undefined for infinite_future() and infinite_past().
        int offset;             // seconds east of UTC
        bool is_dst;            // is offset non-standard?
        const char *zone_abbr;  // time-zone abbreviation (e.g., "PST")
    };

    // time_zone::At(abel_time)
    //
    // Returns the civil time for this time_zone at a certain `abel::abel_time`.
    // If the input time is infinite, the output civil second will be set to
    // chrono_second::max() or min(), and the subsecond will be infinite.
    //
    // Example:
    //
    //   const auto epoch = lax.At(abel::unix_epoch());
    //   // epoch.cs == 1969-12-31 16:00:00
    //   // epoch.subsecond == abel::zero_duration()
    //   // epoch.offset == -28800
    //   // epoch.is_dst == false
    //   // epoch.abbr == "PST"
    chrono_info at (abel_time t) const;

    // time_zone::time_info
    //
    // Information about the absolute times corresponding to a civil time.
    // (Subseconds must be handled separately.)
    //
    // It is possible for a caller to pass a civil-time value that does
    // not represent an actual or unique instant in time (due to a shift
    // in UTC offset in the time_zone, which results in a discontinuity in
    // the civil-time components). For example, a daylight-saving-time
    // transition skips or repeats civil times---in the United States,
    // March 13, 2011 02:15 never occurred, while November 6, 2011 01:15
    // occurred twice---so requests for such times are not well-defined.
    // To account for these possibilities, `abel::time_zone::time_info` is
    // richer than just a single `abel::abel_time`.
    struct time_info {
        enum chrono_kind {
            UNIQUE,    // the civil time was singular (pre == trans == post)
            SKIPPED,   // the civil time did not exist (pre >= trans > post)
            REPEATED,  // the civil time was ambiguous (pre < trans <= post)
        } kind;
        abel_time pre;    // time calculated using the pre-transition offset
        abel_time trans;  // when the civil-time discontinuity occurred
        abel_time post;   // time calculated using the post-transition offset
    };

    // time_zone::At(chrono_second)
    //
    // Returns an `abel::time_info` containing the absolute time(s) for this
    // time_zone at an `abel::chrono_second`. When the civil time is skipped or
    // repeated, returns times calculated using the pre-transition and post-
    // transition UTC offsets, plus the transition time itself.
    //
    // Examples:
    //
    //   // A unique civil time
    //   const auto jan01 = lax.At(abel::chrono_second(2011, 1, 1, 0, 0, 0));
    //   // jan01.kind == time_zone::time_info::UNIQUE
    //   // jan01.pre    is 2011-01-01 00:00:00 -0800
    //   // jan01.trans  is 2011-01-01 00:00:00 -0800
    //   // jan01.post   is 2011-01-01 00:00:00 -0800
    //
    //   // A Spring DST transition, when there is a gap in civil time
    //   const auto mar13 = lax.At(abel::chrono_second(2011, 3, 13, 2, 15, 0));
    //   // mar13.kind == time_zone::time_info::SKIPPED
    //   // mar13.pre   is 2011-03-13 03:15:00 -0700
    //   // mar13.trans is 2011-03-13 03:00:00 -0700
    //   // mar13.post  is 2011-03-13 01:15:00 -0800
    //
    //   // A Fall DST transition, when civil times are repeated
    //   const auto nov06 = lax.At(abel::chrono_second(2011, 11, 6, 1, 15, 0));
    //   // nov06.kind == time_zone::time_info::REPEATED
    //   // nov06.pre   is 2011-11-06 01:15:00 -0700
    //   // nov06.trans is 2011-11-06 01:00:00 -0800
    //   // nov06.post  is 2011-11-06 01:15:00 -0800
    time_info at (chrono_second ct) const;

    // time_zone::next_transition()
    // time_zone::prev_transition()
    //
    // Finds the time of the next/previous offset change in this time zone.
    //
    // By definition, `next_transition(t, &trans)` returns false when `t` is
    // `infinite_future()`, and `prev_transition(t, &trans)` returns false
    // when `t` is `infinite_past()`. If the zone has no transitions, the
    // result will also be false no matter what the argument.
    //
    // Otherwise, when `t` is `infinite_past()`, `next_transition(t, &trans)`
    // returns true and sets `trans` to the first recorded transition. Chains
    // of calls to `next_transition()/prev_transition()` will eventually return
    // false, but it is unspecified exactly when `next_transition(t, &trans)`
    // jumps to false, or what time is set by `prev_transition(t, &trans)` for
    // a very distant `t`.
    //
    // Note: Enumeration of time-zone transitions is for informational purposes
    // only. Modern time-related code should not care about when offset changes
    // occur.
    //
    // Example:
    //   abel::time_zone nyc;
    //   if (!abel::load_time_zone("America/New_York", &nyc)) { ... }
    //   const auto now = abel::now();
    //   auto t = abel::infinite_past();
    //   abel::time_zone::chrono_transition trans;
    //   while (t <= now && nyc.next_transition(t, &trans)) {
    //     // transition: trans.from -> trans.to
    //     t = nyc.At(trans.to).trans;
    //   }
    struct chrono_transition {
        chrono_second from;  // the civil time we jump from
        chrono_second to;    // the civil time we jump to
    };
    bool next_transition (abel_time t, chrono_transition *trans) const;
    bool prev_transition (abel_time t, chrono_transition *trans) const;

    template<typename H>
    friend H AbelHashValue (H h, time_zone tz) {
        return H::combine(std::move(h), tz.cz_);
    }

private:
    friend bool operator == (time_zone a, time_zone b) { return a.cz_ == b.cz_; }
    friend bool operator != (time_zone a, time_zone b) { return a.cz_ != b.cz_; }
    friend std::ostream &operator << (std::ostream &os, time_zone tz) {
        return os << tz.name();
    }

    abel::chrono_internal::time_zone cz_;
};

// load_time_zone()
//
// Loads the named zone. May perform I/O on the initial load of the named
// zone. If the name is invalid, or some other kind of error occurs, returns
// `false` and `*tz` is set to the UTC time zone.
ABEL_FORCE_INLINE bool load_time_zone (const std::string &name, time_zone *tz) {
    if (name == "localtime") {
        *tz = time_zone(abel::chrono_internal::local_time_zone());
        return true;
    }
    abel::chrono_internal::time_zone cz;
    const bool b = chrono_internal::load_time_zone(name, &cz);
    *tz = time_zone(cz);
    return b;
}

// fixed_time_zone()
//
// Returns a time_zone that is a fixed offset (seconds east) from UTC.
// Note: If the absolute value of the offset is greater than 24 hours
// you'll get UTC (i.e., no offset) instead.
ABEL_FORCE_INLINE time_zone fixed_time_zone (int seconds) {
    return time_zone(
        abel::chrono_internal::fixed_time_zone(std::chrono::seconds(seconds)));
}

// utc_time_zone()
//
// Convenience method returning the UTC time zone.
ABEL_FORCE_INLINE time_zone utc_time_zone () {
    return time_zone(abel::chrono_internal::utc_time_zone());
}

// local_time_zone()
//
// Convenience method returning the local time zone, or UTC if there is
// no configured local zone.  Warning: Be wary of using local_time_zone(),
// and particularly so in a server process, as the zone configured for the
// local machine should be irrelevant.  Prefer an explicit zone name.
ABEL_FORCE_INLINE time_zone local_time_zone () {
    return time_zone(abel::chrono_internal::local_time_zone());
}

// to_chrono_second()
// to_chrono_minute()
// to_chrono_hour()
// to_chrono_day()
// to_chrono_month()
// to_chrono_year()
//
// Helpers for time_zone::At(abel_time) to return particularly aligned civil times.
//
// Example:
//
//   abel::abel_time t = ...;
//   abel::time_zone tz = ...;
//   const auto cd = abel::to_chrono_day(t, tz);
ABEL_FORCE_INLINE chrono_second to_chrono_second (abel_time t, time_zone tz) {
    return tz.at(t).cs;  // already a chrono_second
}
ABEL_FORCE_INLINE chrono_minute to_chrono_minute (abel_time t, time_zone tz) {
    return chrono_minute(tz.at(t).cs);
}
ABEL_FORCE_INLINE chrono_hour to_chrono_hour (abel_time t, time_zone tz) {
    return chrono_hour(tz.at(t).cs);
}
ABEL_FORCE_INLINE chrono_day to_chrono_day (abel_time t, time_zone tz) {
    return chrono_day(tz.at(t).cs);
}
ABEL_FORCE_INLINE chrono_month to_chrono_month (abel_time t, time_zone tz) {
    return chrono_month(tz.at(t).cs);
}
ABEL_FORCE_INLINE chrono_year to_chrono_year (abel_time t, time_zone tz) {
    return chrono_year(tz.at(t).cs);
}

// from_chrono()
//
// Helper for time_zone::At(chrono_second) that provides "order-preserving
// semantics." If the civil time maps to a unique time, that time is
// returned. If the civil time is repeated in the given time zone, the
// time using the pre-transition offset is returned. Otherwise, the
// civil time is skipped in the given time zone, and the transition time
// is returned. This means that for any two civil times, ct1 and ct2,
// (ct1 < ct2) => (from_chrono(ct1) <= from_chrono(ct2)), the equal case
// being when two non-existent civil times map to the same transition time.
//
// Note: Accepts civil times of any alignment.
ABEL_FORCE_INLINE abel_time from_chrono (chrono_second ct, time_zone tz) {
    const auto ti = tz.at(ct);
    if (ti.kind == time_zone::time_info::SKIPPED)
        return ti.trans;
    return ti.pre;
}

// time_conversion
//
// An `abel::time_conversion` represents the conversion of year, month, day,
// hour, minute, and second values (i.e., a civil time), in a particular
// `abel::time_zone`, to a time instant (an absolute time), as returned by
// `abel::convert_date_time()`. Legacy version of `abel::time_zone::time_info`.
//
// Deprecated. Use `abel::time_zone::time_info`.
struct
time_conversion {
    abel_time pre;    // time calculated using the pre-transition offset
    abel_time trans;  // when the civil-time discontinuity occurred
    abel_time post;   // time calculated using the post-transition offset

    enum Kind {
        UNIQUE,    // the civil time was singular (pre == trans == post)
        SKIPPED,   // the civil time did not exist
        REPEATED,  // the civil time was ambiguous
    };
    Kind kind;

    bool normalized;  // input values were outside their valid ranges
};

// convert_date_time()
//
// Legacy version of `abel::time_zone::At(abel::chrono_second)` that takes
// the civil time as six, separate values (YMDHMS).
//
// The input month, day, hour, minute, and second values can be outside
// of their valid ranges, in which case they will be "normalized" during
// the conversion.
//
// Example:
//
//   // "October 32" normalizes to "November 1".
//   abel::time_conversion tc =
//       abel::convert_date_time(2013, 10, 32, 8, 30, 0, lax);
//   // tc.kind == time_conversion::UNIQUE && tc.normalized == true
//   // abel::to_chrono_day(tc.pre, tz).month() == 11
//   // abel::to_chrono_day(tc.pre, tz).day() == 1
//
// Deprecated. Use `abel::time_zone::At(chrono_second)`.
time_conversion convert_date_time (int64_t year, int mon, int day, int hour,
                                  int min, int sec, time_zone tz);

// format_date_time()
//
// A convenience wrapper for `abel::convert_date_time()` that simply returns
// the "pre" `abel::abel_time`.  That is, the unique result, or the instant that
// is correct using the pre-transition offset (as if the transition never
// happened).
//
// Example:
//
//   abel::abel_time t = abel::format_date_time(2017, 9, 26, 9, 30, 0, lax);
//   // t = 2017-09-26 09:30:00 -0700
//
// Deprecated. Use `abel::from_chrono(chrono_second, time_zone)`. Note that the
// behavior of `from_chrono()` differs from `format_date_time()` for skipped civil
// times. If you care about that see `abel::time_zone::At(abel::chrono_second)`.
ABEL_FORCE_INLINE abel_time format_date_time (int64_t year, int mon, int day, int hour,
                                              int min, int sec, time_zone tz) {
    return convert_date_time(year, mon, day, hour, min, sec, tz).pre;
}

// from_tm()
//
// Converts the `tm_year`, `tm_mon`, `tm_mday`, `tm_hour`, `tm_min`, and
// `tm_sec` fields to an `abel::abel_time` using the given time zone. See ctime(3)
// for a description of the expected values of the tm fields. If the indicated
// time instant is not unique (see `abel::time_zone::at(abel::chrono_second)`
// above), the `tm_isdst` field is consulted to select the desired instant
// (`tm_isdst` > 0 means DST, `tm_isdst` == 0 means no DST, `tm_isdst` < 0
// means use the post-transition offset).
abel_time from_tm (const struct tm &tm, time_zone tz);

// to_tm()
//
// Converts the given `abel::abel_time` to a struct tm using the given time zone.
// See ctime(3) for a description of the values of the tm fields.
struct tm to_tm (abel_time t, time_zone tz);

inline struct tm local_tm (abel_time t) {
    return to_tm(t, abel::local_time_zone());
}

inline struct tm utc_tm (abel_time t) {
    return to_tm(t, abel::utc_time_zone());
}

inline bool operator==(const std::tm &tm1, const std::tm &tm2)
{
    return (tm1.tm_sec == tm2.tm_sec && tm1.tm_min == tm2.tm_min && tm1.tm_hour == tm2.tm_hour && tm1.tm_mday == tm2.tm_mday &&
        tm1.tm_mon == tm2.tm_mon && tm1.tm_year == tm2.tm_year && tm1.tm_isdst == tm2.tm_isdst);
}

inline bool operator!=(const std::tm &tm1, const std::tm &tm2)
{
    return !(tm1 == tm2);
}

// RFC3339_full
// RFC3339_sec
//
// format_time()/parse_time() format specifiers for RFC3339 date/time strings,
// with trailing zeros trimmed or with fractional seconds omitted altogether.
//
// Note that RFC3339_sec[] matches an ISO 8601 extended format for date and
// time with UTC offset.  Also note the use of "%Y": RFC3339 mandates that
// years have exactly four digits, but we allow them to take their natural
// width.
extern const char RFC3339_full[];  // %Y-%m-%dT%H:%M:%E*S%Ez
extern const char RFC3339_sec[];   // %Y-%m-%dT%H:%M:%S%Ez

// RFC1123_full
// RFC1123_no_wday
//
// format_time()/parse_time() format specifiers for RFC1123 date/time strings.
extern const char RFC1123_full[];     // %a, %d %b %E4Y %H:%M:%S %z
extern const char RFC1123_no_wday[];  // %d %b %E4Y %H:%M:%S %z

// format_time()
//
// Formats the given `abel::abel_time` in the `abel::time_zone` according to the
// provided format string. Uses strftime()-like formatting options, with
// the following extensions:
//
//   - %Ez  - RFC3339-compatible numeric UTC offset (+hh:mm or -hh:mm)
//   - %E*z - Full-resolution numeric UTC offset (+hh:mm:ss or -hh:mm:ss)
//   - %E#S - seconds with # digits of fractional precision
//   - %E*S - seconds with full fractional precision (a literal '*')
//   - %E#f - Fractional seconds with # digits of precision
//   - %E*f - Fractional seconds with full precision (a literal '*')
//   - %E4Y - Four-character years (-999 ... -001, 0000, 0001 ... 9999)
//
// Note that %E0S behaves like %S, and %E0f produces no characters.  In
// contrast %E*f always produces at least one digit, which may be '0'.
//
// Note that %Y produces as many characters as it takes to fully render the
// year.  A year outside of [-999:9999] when formatted with %E4Y will produce
// more than four characters, just like %Y.
//
// We recommend that format strings include the UTC offset (%z, %Ez, or %E*z)
// so that the result uniquely identifies a time instant.
//
// Example:
//
//   abel::chrono_second cs(2013, 1, 2, 3, 4, 5);
//   abel::abel_time t = abel::from_chrono(cs, lax);
//   std::string f = abel::format_time("%H:%M:%S", t, lax);  // "03:04:05"
//   f = abel::format_time("%H:%M:%E3S", t, lax);  // "03:04:05.000"
//
// Note: If the given `abel::abel_time` is `abel::infinite_future()`, the returned
// string will be exactly "infinite-future". If the given `abel::abel_time` is
// `abel::infinite_past()`, the returned string will be exactly "infinite-past".
// In both cases the given format string and `abel::time_zone` are ignored.
//
std::string format_time (const std::string &format, abel_time t, time_zone tz);

// Convenience functions that format the given time using the RFC3339_full
// format.  The first overload uses the provided time_zone, while the second
// uses local_time_zone().
std::string format_time (abel_time t, time_zone tz);
std::string format_time (abel_time t);

// Output stream operator.
ABEL_FORCE_INLINE std::ostream &operator << (std::ostream &os, abel_time t) {
    return os << format_time(t);
}

// parse_time()
//
// Parses an input string according to the provided format string and
// returns the corresponding `abel::abel_time`. Uses strftime()-like formatting
// options, with the same extensions as format_time(), but with the
// exceptions that %E#S is interpreted as %E*S, and %E#f as %E*f.  %Ez
// and %E*z also accept the same inputs.
//
// %Y consumes as many numeric characters as it can, so the matching data
// should always be terminated with a non-numeric.  %E4Y always consumes
// exactly four characters, including any sign.
//
// Unspecified fields are taken from the default date and time of ...
//
//   "1970-01-01 00:00:00.0 +0000"
//
// For example, parsing a string of "15:45" (%H:%M) will return an abel::abel_time
// that represents "1970-01-01 15:45:00.0 +0000".
//
// Note that since parse_time() returns time instants, it makes the most sense
// to parse fully-specified date/time strings that include a UTC offset (%z,
// %Ez, or %E*z).
//
// Note also that `abel::parse_time()` only heeds the fields year, month, day,
// hour, minute, (fractional) second, and UTC offset.  Other fields, like
// weekday (%a or %A), while parsed for syntactic validity, are ignored
// in the conversion.
//
// Date and time fields that are out-of-range will be treated as errors
// rather than normalizing them like `abel::chrono_second` does.  For example,
// it is an error to parse the date "Oct 32, 2013" because 32 is out of range.
//
// A leap second of ":60" is normalized to ":00" of the following minute
// with fractional seconds discarded.  The following table shows how the
// given seconds and subseconds will be parsed:
//
//   "59.x" -> 59.x  // exact
//   "60.x" -> 00.0  // normalized
//   "00.x" -> 00.x  // exact
//
// Errors are indicated by returning false and assigning an error message
// to the "err" out param if it is non-null.
//
// Note: If the input string is exactly "infinite-future", the returned
// `abel::abel_time` will be `abel::infinite_future()` and `true` will be returned.
// If the input string is "infinite-past", the returned `abel::abel_time` will be
// `abel::infinite_past()` and `true` will be returned.
//
bool parse_time (const std::string &format, const std::string &input, abel_time *time,
                 std::string *err);

// Like parse_time() above, but if the format string does not contain a UTC
// offset specification (%z/%Ez/%E*z) then the input is interpreted in the
// given time_zone.  This means that the input, by itself, does not identify a
// unique instant.  Being time-zone dependent, it also admits the possibility
// of ambiguity or non-existence, in which case the "pre" time (as defined
// by time_zone::time_info) is returned.  For these reasons we recommend that
// all date/time strings include a UTC offset so they're context independent.
bool parse_time (const std::string &format, const std::string &input, time_zone tz,
                 abel_time *time, std::string *err);

// ============================================================================
// Implementation Details Follow
// ============================================================================

namespace chrono_internal {

// Creates a duration with a given representation.
// REQUIRES: hi,lo is a valid representation of a duration as specified
// in time/duration.cc.
constexpr duration make_duration (int64_t hi, uint32_t lo = 0) {
    return duration(hi, lo);
}

constexpr duration make_duration (int64_t hi, int64_t lo) {
    return make_duration(hi, static_cast<uint32_t>(lo));
}

// Make a duration value from a floating-point number, as long as that number
// is in the range [ 0 .. numeric_limits<int64_t>::max ), that is, as long as
// it's positive and can be converted to int64_t without risk of UB.
ABEL_FORCE_INLINE duration make_pos_double_duration (double n) {
    const int64_t int_secs = static_cast<int64_t>(n);
    const uint32_t ticks =
        static_cast<uint32_t>((n - int_secs) * kTicksPerSecond + 0.5);
    return ticks < kTicksPerSecond
           ? make_duration(int_secs, ticks)
           : make_duration(int_secs + 1, ticks - kTicksPerSecond);
}

// Creates a normalized duration from an almost-normalized (sec,ticks)
// pair. sec may be positive or negative.  ticks must be in the range
// -kTicksPerSecond < *ticks < kTicksPerSecond.  If ticks is negative it
// will be normalized to a positive value in the resulting duration.
constexpr duration make_normalized_duration (int64_t sec, int64_t ticks) {
    return (ticks < 0) ? make_duration(sec - 1, ticks + kTicksPerSecond)
                       : make_duration(sec, ticks);
}

// Provide access to the duration representation.
constexpr int64_t get_rep_hi (duration d) { return d.rep_hi_; }
constexpr uint32_t get_rep_lo (duration d) { return d.rep_lo_; }

// Returns true iff d is positive or negative infinity.
constexpr bool is_infinite_duration (duration d) { return get_rep_lo(d) == ~0U; }

// Returns an infinite duration with the opposite sign.
// REQUIRES: is_infinite_duration(d)
constexpr duration opposite_infinity (duration d) {
    return get_rep_hi(d) < 0
           ? make_duration((std::numeric_limits<int64_t>::max)(), ~0U)
           : make_duration((std::numeric_limits<int64_t>::min)(), ~0U);
}

// Returns (-n)-1 (equivalently -(n+1)) without avoidable overflow.
constexpr int64_t negate_and_subtract_one (int64_t n) {
    // Note: Good compilers will optimize this expression to ~n when using
    // a two's-complement representation (which is required for int64_t).
    return (n < 0) ? -(n + 1) : (-n) - 1;
}

// Map between a abel_time and a duration since the Unix epoch.  Note that these
// functions depend on the above mentioned choice of the Unix epoch for the
// abel_time representation (and both need to be abel_time friends).  Without this
// knowledge, we would need to add-in/subtract-out unix_epoch() respectively.
constexpr abel_time from_unix_duration (duration d) { return abel_time(d); }
constexpr duration to_unix_duration (abel_time t) { return t.rep_; }

template<std::intmax_t N>
constexpr duration from_int64 (int64_t v, std::ratio<1, N>) {
    static_assert(0 < N && N <= 1000 * 1000 * 1000, "Unsupported ratio");
    // Subsecond ratios cannot overflow.
    return make_normalized_duration(
        v / N, v % N * kTicksPerNanosecond * 1000 * 1000 * 1000 / N);
}
constexpr duration from_int64 (int64_t v, std::ratio<60>) {
    return (v <= (std::numeric_limits<int64_t>::max)() / 60 &&
        v >= (std::numeric_limits<int64_t>::min)() / 60)
           ? make_duration(v * 60)
           : v > 0 ? infinite_duration() : -infinite_duration();
}
constexpr duration from_int64 (int64_t v, std::ratio<3600>) {
    return (v <= (std::numeric_limits<int64_t>::max)() / 3600 &&
        v >= (std::numeric_limits<int64_t>::min)() / 3600)
           ? make_duration(v * 3600)
           : v > 0 ? infinite_duration() : -infinite_duration();
}

// is_valid_rep64<T>(0) is true if the expression `int64_t{std::declval<T>()}` is
// valid. That is, if a T can be assigned to an int64_t without narrowing.
template<typename T>
constexpr auto is_valid_rep64 (int) -> decltype(int64_t {std::declval<T>()} == 0) {
    return true;
}
template<typename T>
constexpr auto is_valid_rep64 (char) -> bool {
    return false;
}

// Converts a std::chrono::duration to an abel::duration.
template<typename Rep, typename Period>
constexpr duration from_chrono (const std::chrono::duration<Rep, Period> &d) {
    static_assert(is_valid_rep64<Rep>(0), "duration::rep is invalid");
    return from_int64(int64_t {d.count()}, Period {});
}

template<typename Ratio>
int64_t to_int64 (duration d, Ratio) {
    // Note: This may be used on MSVC, which may have a system_clock period of
    // std::ratio<1, 10 * 1000 * 1000>
    return to_int64_seconds(d * Ratio::den / Ratio::num);
}
// Fastpath implementations for the 6 common duration units.
ABEL_FORCE_INLINE int64_t to_int64 (duration d, std::nano) {
    return to_int64_nanoseconds(d);
}
ABEL_FORCE_INLINE int64_t to_int64 (duration d, std::micro) {
    return to_int64_microseconds(d);
}
ABEL_FORCE_INLINE int64_t to_int64 (duration d, std::milli) {
    return to_int64_milliseconds(d);
}
ABEL_FORCE_INLINE int64_t to_int64 (duration d, std::ratio<1>) {
    return to_int64_seconds(d);
}
ABEL_FORCE_INLINE int64_t to_int64 (duration d, std::ratio<60>) {
    return to_int64_minutes(d);
}
ABEL_FORCE_INLINE int64_t to_int64 (duration d, std::ratio<3600>) {
    return to_int64_hours(d);
}

// Converts an abel::duration to a chrono duration of type T.
template<typename T>
T to_chrono_duration (duration d) {
    using Rep = typename T::rep;
    using Period = typename T::period;
    static_assert(is_valid_rep64<Rep>(0), "duration::rep is invalid");
    if (chrono_internal::is_infinite_duration(d))
        return d < zero_duration() ? (T::min)() : (T::max)();
    const auto v = to_int64(d, Period {});
    if (v > (std::numeric_limits<Rep>::max)())
        return (T::max)();
    if (v < (std::numeric_limits<Rep>::min)())
        return (T::min)();
    return T {v};
}

}  // namespace chrono_internal

constexpr duration nanoseconds (int64_t n) {
    return chrono_internal::from_int64(n, std::nano {});
}
constexpr duration microseconds (int64_t n) {
    return chrono_internal::from_int64(n, std::micro {});
}
constexpr duration milliseconds (int64_t n) {
    return chrono_internal::from_int64(n, std::milli {});
}
constexpr duration seconds (int64_t n) {
    return chrono_internal::from_int64(n, std::ratio<1> {});
}
constexpr duration minutes (int64_t n) {
    return chrono_internal::from_int64(n, std::ratio<60> {});
}
constexpr duration hours (int64_t n) {
    return chrono_internal::from_int64(n, std::ratio<3600> {});
}

constexpr bool operator < (duration lhs, duration rhs) {
    return chrono_internal::get_rep_hi(lhs) != chrono_internal::get_rep_hi(rhs)
           ? chrono_internal::get_rep_hi(lhs) < chrono_internal::get_rep_hi(rhs)
           : chrono_internal::get_rep_hi(lhs) ==
            (std::numeric_limits<int64_t>::min)()
             ? chrono_internal::get_rep_lo(lhs) + 1 <
                chrono_internal::get_rep_lo(rhs) + 1
             : chrono_internal::get_rep_lo(lhs) <
                chrono_internal::get_rep_lo(rhs);
}

constexpr bool operator == (duration lhs, duration rhs) {
    return chrono_internal::get_rep_hi(lhs) == chrono_internal::get_rep_hi(rhs) &&
        chrono_internal::get_rep_lo(lhs) == chrono_internal::get_rep_lo(rhs);
}

constexpr duration operator - (duration d) {
    // This is a little interesting because of the special cases.
    //
    // If rep_lo_ is zero, we have it easy; it's safe to negate rep_hi_, we're
    // dealing with an integral number of seconds, and the only special case is
    // the maximum negative finite duration, which can't be negated.
    //
    // Infinities stay infinite, and just change direction.
    //
    // Finally we're in the case where rep_lo_ is non-zero, and we can borrow
    // a second's worth of ticks and avoid overflow (as negating int64_t-min + 1
    // is safe).
    return chrono_internal::get_rep_lo(d) == 0
           ? chrono_internal::get_rep_hi(d) ==
            (std::numeric_limits<int64_t>::min)()
             ? infinite_duration()
             : chrono_internal::make_duration(-chrono_internal::get_rep_hi(d))
           : chrono_internal::is_infinite_duration(d)
             ? chrono_internal::opposite_infinity(d)
             : chrono_internal::make_duration(
                chrono_internal::negate_and_subtract_one(
                    chrono_internal::get_rep_hi(d)),
                chrono_internal::kTicksPerSecond -
                    chrono_internal::get_rep_lo(d));
}

constexpr duration infinite_duration () {
    return chrono_internal::make_duration((std::numeric_limits<int64_t>::max)(),
                                         ~0U);
}

constexpr duration from_chrono (const std::chrono::nanoseconds &d) {
    return chrono_internal::from_chrono(d);
}
constexpr duration from_chrono (const std::chrono::microseconds &d) {
    return chrono_internal::from_chrono(d);
}
constexpr duration from_chrono (const std::chrono::milliseconds &d) {
    return chrono_internal::from_chrono(d);
}
constexpr duration from_chrono (const std::chrono::seconds &d) {
    return chrono_internal::from_chrono(d);
}
constexpr duration from_chrono (const std::chrono::minutes &d) {
    return chrono_internal::from_chrono(d);
}
constexpr duration from_chrono (const std::chrono::hours &d) {
    return chrono_internal::from_chrono(d);
}

constexpr abel_time from_unix_nanos (int64_t ns) {
    return chrono_internal::from_unix_duration(nanoseconds(ns));
}

constexpr abel_time from_unix_micros (int64_t us) {
    return chrono_internal::from_unix_duration(microseconds(us));
}

constexpr abel_time from_unix_millis (int64_t ms) {
    return chrono_internal::from_unix_duration(milliseconds(ms));
}

constexpr abel_time from_unix_seconds (int64_t s) {
    return chrono_internal::from_unix_duration(seconds(s));
}

constexpr abel_time from_time_t (time_t t) {
    return chrono_internal::from_unix_duration(seconds(t));
}

inline int utc_minutes_offset(const std::tm &tm)
{

#ifdef _WIN32
    #if _WIN32_WINNT < _WIN32_WINNT_WS08
    TIME_ZONE_INFORMATION tzinfo;
    auto rv = GetTimeZoneInformation(&tzinfo);
#else
    DYNAMIC_TIME_ZONE_INFORMATION tzinfo;
    auto rv = GetDynamicTimeZoneInformation(&tzinfo);
#endif
    if (rv == TIME_ZONE_ID_INVALID)
        throw abel_log::spdlog_ex("Failed getting timezone info. ", errno);

    int offset = -tzinfo.Bias;
    if (tm.tm_isdst)
    {
        offset -= tzinfo.DaylightBias;
    }
    else
    {
        offset -= tzinfo.StandardBias;
    }
    return offset;
#else

#if defined(sun) || defined(__sun) || defined(_AIX)
    // 'tm_gmtoff' field is BSD extension and it's missing on SunOS/Solaris
    struct helper
    {
        static long int calculate_gmt_offset(const std::tm &localtm = details::os::localtime(), const std::tm &gmtm = details::os::gmtime())
        {
            int local_year = localtm.tm_year + (1900 - 1);
            int gmt_year = gmtm.tm_year + (1900 - 1);

            long int days = (
                // difference in day of year
                localtm.tm_yday -
                gmtm.tm_yday

                // + intervening leap days
                + ((local_year >> 2) - (gmt_year >> 2)) - (local_year / 100 - gmt_year / 100) +
                ((local_year / 100 >> 2) - (gmt_year / 100 >> 2))

                // + difference in years * 365 */
                + (long int)(local_year - gmt_year) * 365);

            long int hours = (24 * days) + (localtm.tm_hour - gmtm.tm_hour);
            long int mins = (60 * hours) + (localtm.tm_min - gmtm.tm_min);
            long int secs = (60 * mins) + (localtm.tm_sec - gmtm.tm_sec);

            return secs;
        }
    };

    auto offset_seconds = helper::calculate_gmt_offset(tm);
#else
    auto offset_seconds = tm.tm_gmtoff;
#endif

    return static_cast<int>(offset_seconds / 60);
#endif
}

}
#endif //ABEL_CHRONO_TIME_H_
