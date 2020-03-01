//

// The implementation of the abel::duration class, which is declared in
// //abel/time.h.  This class behaves like a numeric type; it has no public
// methods and is used only through the operators defined here.
//
// Implementation notes:
//
// An abel::duration is represented as
//
//   rep_hi_ : (int64_t)  Whole seconds
//   rep_lo_ : (uint32_t) Fractions of a second
//
// The seconds value (rep_hi_) may be positive or negative as appropriate.
// The fractional seconds (rep_lo_) is always a positive offset from rep_hi_.
// The API for duration guarantees at least nanosecond resolution, which
// means rep_lo_ could have a max value of 1B - 1 if it stored nanoseconds.
// However, to utilize more of the available 32 bits of space in rep_lo_,
// we instead store quarters of a nanosecond in rep_lo_ resulting in a max
// value of 4B - 1.  This allows us to correctly handle calculations like
// 0.5 nanos + 0.5 nanos = 1 nano.  The following example shows the actual
// duration rep using quarters of a nanosecond.
//
//    2.5 sec = {rep_hi_=2,  rep_lo_=2000000000}  // lo = 4 * 500000000
//   -2.5 sec = {rep_hi_=-3, rep_lo_=2000000000}
//
// Infinite durations are represented as Durations with the rep_lo_ field set
// to all 1s.
//
//   +infinite_duration:
//     rep_hi_ : kint64max
//     rep_lo_ : ~0U
//
//   -infinite_duration:
//     rep_hi_ : kint64min
//     rep_lo_ : ~0U
//
// Arithmetic overflows/underflows to +/- infinity and saturates.

#if defined(_MSC_VER)
#include <winsock2.h>  // for timeval
#endif

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cerrno>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <functional>
#include <limits>
#include <string>

#include <abel/math/bit_cast.h>
#include <abel/numeric/int128.h>
#include <abel/chrono/time.h>

namespace abel {


    namespace {

        using chrono_internal::kTicksPerNanosecond;
        using chrono_internal::kTicksPerSecond;

        constexpr int64_t kint64max = std::numeric_limits<int64_t>::max();
        constexpr int64_t kint64min = std::numeric_limits<int64_t>::min();

// Can't use std::isinfinite() because it doesn't exist on windows.
        ABEL_FORCE_INLINE bool IsFinite(double d) {
            if (std::isnan(d)) return false;
            return d != std::numeric_limits<double>::infinity() &&
                   d != -std::numeric_limits<double>::infinity();
        }

        ABEL_FORCE_INLINE bool IsValidDivisor(double d) {
            if (std::isnan(d)) return false;
            return d != 0.0;
        }

// Can't use std::round() because it is only available in C++11.
// Note that we ignore the possibility of floating-point over/underflow.
        template<typename Double>
        ABEL_FORCE_INLINE double Round(Double d) {
            return d < 0 ? std::ceil(d - 0.5) : std::floor(d + 0.5);
        }

// *sec may be positive or negative.  *ticks must be in the range
// -kTicksPerSecond < *ticks < kTicksPerSecond.  If *ticks is negative it
// will be normalized to a positive value by adjusting *sec accordingly.
        ABEL_FORCE_INLINE void NormalizeTicks(int64_t *sec, int64_t *ticks) {
            if (*ticks < 0) {
                --*sec;
                *ticks += kTicksPerSecond;
            }
        }

// Makes a uint128 from the absolute value of the given scalar.
        ABEL_FORCE_INLINE uint128 MakeU128(int64_t a) {
            uint128 u128 = 0;
            if (a < 0) {
                ++u128;
                ++a;  // Makes it safe to negate 'a'
                a = -a;
            }
            u128 += static_cast<uint64_t>(a);
            return u128;
        }

// Makes a uint128 count of ticks out of the absolute value of the duration.
        ABEL_FORCE_INLINE uint128 MakeU128Ticks(duration d) {
            int64_t rep_hi = chrono_internal::get_rep_hi(d);
            uint32_t rep_lo = chrono_internal::get_rep_lo(d);
            if (rep_hi < 0) {
                ++rep_hi;
                rep_hi = -rep_hi;
                rep_lo = kTicksPerSecond - rep_lo;
            }
            uint128 u128 = static_cast<uint64_t>(rep_hi);
            u128 *= static_cast<uint64_t>(kTicksPerSecond);
            u128 += rep_lo;
            return u128;
        }

// Breaks a uint128 of ticks into a duration.
        ABEL_FORCE_INLINE duration MakeDurationFromU128(uint128 u128, bool is_neg) {
            int64_t rep_hi;
            uint32_t rep_lo;
            const uint64_t h64 = Uint128High64(u128);
            const uint64_t l64 = Uint128Low64(u128);
            if (h64 == 0) {  // fastpath
                const uint64_t hi = l64 / kTicksPerSecond;
                rep_hi = static_cast<int64_t>(hi);
                rep_lo = static_cast<uint32_t>(l64 - hi * kTicksPerSecond);
            } else {
                // kMaxRepHi64 is the high 64 bits of (2^63 * kTicksPerSecond).
                // Any positive tick count whose high 64 bits are >= kMaxRepHi64
                // is not representable as a duration.  A negative tick count can
                // have its high 64 bits == kMaxRepHi64 but only when the low 64
                // bits are all zero, otherwise it is not representable either.
                const uint64_t kMaxRepHi64 = 0x77359400UL;
                if (h64 >= kMaxRepHi64) {
                    if (is_neg && h64 == kMaxRepHi64 && l64 == 0) {
                        // Avoid trying to represent -kint64min below.
                        return chrono_internal::make_duration(kint64min);
                    }
                    return is_neg ? -infinite_duration() : infinite_duration();
                }
                const uint128 kTicksPerSecond128 = static_cast<uint64_t>(kTicksPerSecond);
                const uint128 hi = u128 / kTicksPerSecond128;
                rep_hi = static_cast<int64_t>(Uint128Low64(hi));
                rep_lo =
                        static_cast<uint32_t>(Uint128Low64(u128 - hi * kTicksPerSecond128));
            }
            if (is_neg) {
                rep_hi = -rep_hi;
                if (rep_lo != 0) {
                    --rep_hi;
                    rep_lo = kTicksPerSecond - rep_lo;
                }
            }
            return chrono_internal::make_duration(rep_hi, rep_lo);
        }

// Convert between int64_t and uint64_t, preserving representation. This
// allows us to do arithmetic in the unsigned domain, where overflow has
// well-defined behavior. See operator+=() and operator-=().
//
// C99 7.20.1.1.1, as referenced by C++11 18.4.1.2, says, "The typedef
// name intN_t designates a signed integer type with width N, no padding
// bits, and a two's complement representation." So, we can convert to
// and from the corresponding uint64_t value using a bit cast.
        ABEL_FORCE_INLINE uint64_t EncodeTwosComp(int64_t v) {
            return abel::bit_cast<uint64_t>(v);
        }

        ABEL_FORCE_INLINE int64_t DecodeTwosComp(uint64_t v) { return abel::bit_cast<int64_t>(v); }

// Note: The overflow detection in this function is done using greater/less *or
// equal* because kint64max/min is too large to be represented exactly in a
// double (which only has 53 bits of precision). In order to avoid assigning to
// rep->hi a double value that is too large for an int64_t (and therefore is
// undefined), we must consider computations that equal kint64max/min as a
// double as overflow cases.
        ABEL_FORCE_INLINE bool SafeAddRepHi(double a_hi, double b_hi, duration *d) {
            double c = a_hi + b_hi;
            if (c >= static_cast<double>(kint64max)) {
                *d = infinite_duration();
                return false;
            }
            if (c <= static_cast<double>(kint64min)) {
                *d = -infinite_duration();
                return false;
            }
            *d = chrono_internal::make_duration(c, chrono_internal::get_rep_lo(*d));
            return true;
        }

// A functor that's similar to std::multiplies<T>, except this returns the max
// T value instead of overflowing. This is only defined for uint128.
        template<typename Ignored>
        struct SafeMultiply {
            uint128 operator()(uint128 a, uint128 b) const {
                // b hi is always zero because it originated as an int64_t.
                assert(Uint128High64(b) == 0);
                // Fastpath to avoid the expensive overflow check with division.
                if (Uint128High64(a) == 0) {
                    return (((Uint128Low64(a) | Uint128Low64(b)) >> 32) == 0)
                           ? static_cast<uint128>(Uint128Low64(a) * Uint128Low64(b))
                           : a * b;
                }
                return b == 0 ? b : (a > kuint128max / b) ? kuint128max : a * b;
            }
        };

// Scales (i.e., multiplies or divides, depending on the Operation template)
// the duration d by the int64_t r.
        template<template<typename> class Operation>
        ABEL_FORCE_INLINE duration ScaleFixed(duration d, int64_t r) {
            const uint128 a = MakeU128Ticks(d);
            const uint128 b = MakeU128(r);
            const uint128 q = Operation<uint128>()(a, b);
            const bool is_neg = (chrono_internal::get_rep_hi(d) < 0) != (r < 0);
            return MakeDurationFromU128(q, is_neg);
        }

// Scales (i.e., multiplies or divides, depending on the Operation template)
// the duration d by the double r.
        template<template<typename> class Operation>
        ABEL_FORCE_INLINE duration ScaleDouble(duration d, double r) {
            Operation<double> op;
            double hi_doub = op(chrono_internal::get_rep_hi(d), r);
            double lo_doub = op(chrono_internal::get_rep_lo(d), r);

            double hi_int = 0;
            double hi_frac = std::modf(hi_doub, &hi_int);

            // Moves hi's fractional bits to lo.
            lo_doub /= kTicksPerSecond;
            lo_doub += hi_frac;

            double lo_int = 0;
            double lo_frac = std::modf(lo_doub, &lo_int);

            // Rolls lo into hi if necessary.
            int64_t lo64 = Round(lo_frac * kTicksPerSecond);

            duration ans;
            if (!SafeAddRepHi(hi_int, lo_int, &ans)) return ans;
            int64_t hi64 = chrono_internal::get_rep_hi(ans);
            if (!SafeAddRepHi(hi64, lo64 / kTicksPerSecond, &ans)) return ans;
            hi64 = chrono_internal::get_rep_hi(ans);
            lo64 %= kTicksPerSecond;
            NormalizeTicks(&hi64, &lo64);
            return chrono_internal::make_duration(hi64, lo64);
        }

// Tries to divide num by den as fast as possible by looking for common, easy
// cases. If the division was done, the quotient is in *q and the remainder is
// in *rem and true will be returned.
        ABEL_FORCE_INLINE bool IDivFastPath(const duration num, const duration den, int64_t *q,
                                            duration *rem) {
            // Bail if num or den is an infinity.
            if (chrono_internal::is_infinite_duration(num) ||
                chrono_internal::is_infinite_duration(den))
                return false;

            int64_t num_hi = chrono_internal::get_rep_hi(num);
            uint32_t num_lo = chrono_internal::get_rep_lo(num);
            int64_t den_hi = chrono_internal::get_rep_hi(den);
            uint32_t den_lo = chrono_internal::get_rep_lo(den);

            if (den_hi == 0 && den_lo == kTicksPerNanosecond) {
                // Dividing by 1ns
                if (num_hi >= 0 && num_hi < (kint64max - kTicksPerSecond) / 1000000000) {
                    *q = num_hi * 1000000000 + num_lo / kTicksPerNanosecond;
                    *rem = chrono_internal::make_duration(0, num_lo % den_lo);
                    return true;
                }
            } else if (den_hi == 0 && den_lo == 100 * kTicksPerNanosecond) {
                // Dividing by 100ns (common when converting to Universal time)
                if (num_hi >= 0 && num_hi < (kint64max - kTicksPerSecond) / 10000000) {
                    *q = num_hi * 10000000 + num_lo / (100 * kTicksPerNanosecond);
                    *rem = chrono_internal::make_duration(0, num_lo % den_lo);
                    return true;
                }
            } else if (den_hi == 0 && den_lo == 1000 * kTicksPerNanosecond) {
                // Dividing by 1us
                if (num_hi >= 0 && num_hi < (kint64max - kTicksPerSecond) / 1000000) {
                    *q = num_hi * 1000000 + num_lo / (1000 * kTicksPerNanosecond);
                    *rem = chrono_internal::make_duration(0, num_lo % den_lo);
                    return true;
                }
            } else if (den_hi == 0 && den_lo == 1000000 * kTicksPerNanosecond) {
                // Dividing by 1ms
                if (num_hi >= 0 && num_hi < (kint64max - kTicksPerSecond) / 1000) {
                    *q = num_hi * 1000 + num_lo / (1000000 * kTicksPerNanosecond);
                    *rem = chrono_internal::make_duration(0, num_lo % den_lo);
                    return true;
                }
            } else if (den_hi > 0 && den_lo == 0) {
                // Dividing by positive multiple of 1s
                if (num_hi >= 0) {
                    if (den_hi == 1) {
                        *q = num_hi;
                        *rem = chrono_internal::make_duration(0, num_lo);
                        return true;
                    }
                    *q = num_hi / den_hi;
                    *rem = chrono_internal::make_duration(num_hi % den_hi, num_lo);
                    return true;
                }
                if (num_lo != 0) {
                    num_hi += 1;
                }
                int64_t quotient = num_hi / den_hi;
                int64_t rem_sec = num_hi % den_hi;
                if (rem_sec > 0) {
                    rem_sec -= den_hi;
                    quotient += 1;
                }
                if (num_lo != 0) {
                    rem_sec -= 1;
                }
                *q = quotient;
                *rem = chrono_internal::make_duration(rem_sec, num_lo);
                return true;
            }

            return false;
        }

    }  // namespace

    namespace chrono_internal {

// The 'satq' argument indicates whether the quotient should saturate at the
// bounds of int64_t.  If it does saturate, the difference will spill over to
// the remainder.  If it does not saturate, the remainder remain accurate,
// but the returned quotient will over/underflow int64_t and should not be used.
        int64_t integer_div_duration(bool satq, const duration num, const duration den,
                                     duration *rem) {
            int64_t q = 0;
            if (IDivFastPath(num, den, &q, rem)) {
                return q;
            }

            const bool num_neg = num < zero_duration();
            const bool den_neg = den < zero_duration();
            const bool quotient_neg = num_neg != den_neg;

            if (chrono_internal::is_infinite_duration(num) || den == zero_duration()) {
                *rem = num_neg ? -infinite_duration() : infinite_duration();
                return quotient_neg ? kint64min : kint64max;
            }
            if (chrono_internal::is_infinite_duration(den)) {
                *rem = num;
                return 0;
            }

            const uint128 a = MakeU128Ticks(num);
            const uint128 b = MakeU128Ticks(den);
            uint128 quotient128 = a / b;

            if (satq) {
                // Limits the quotient to the range of int64_t.
                if (quotient128 > uint128(static_cast<uint64_t>(kint64max))) {
                    quotient128 = quotient_neg ? uint128(static_cast<uint64_t>(kint64min))
                                               : uint128(static_cast<uint64_t>(kint64max));
                }
            }

            const uint128 remainder128 = a - quotient128 * b;
            *rem = MakeDurationFromU128(remainder128, num_neg);

            if (!quotient_neg || quotient128 == 0) {
                return Uint128Low64(quotient128) & kint64max;
            }
            // The quotient needs to be negated, but we need to carefully handle
            // quotient128s with the top bit on.
            return -static_cast<int64_t>(Uint128Low64(quotient128 - 1) & kint64max) - 1;
        }

    }  // namespace chrono_internal

//
// Additive operators.
//

    duration &duration::operator+=(duration rhs) {
        if (chrono_internal::is_infinite_duration(*this)) return *this;
        if (chrono_internal::is_infinite_duration(rhs)) return *this = rhs;
        const int64_t orig_rep_hi = rep_hi_;
        rep_hi_ =
                DecodeTwosComp(EncodeTwosComp(rep_hi_) + EncodeTwosComp(rhs.rep_hi_));
        if (rep_lo_ >= kTicksPerSecond - rhs.rep_lo_) {
            rep_hi_ = DecodeTwosComp(EncodeTwosComp(rep_hi_) + 1);
            rep_lo_ -= kTicksPerSecond;
        }
        rep_lo_ += rhs.rep_lo_;
        if (rhs.rep_hi_ < 0 ? rep_hi_ > orig_rep_hi : rep_hi_ < orig_rep_hi) {
            return *this = rhs.rep_hi_ < 0 ? -infinite_duration() : infinite_duration();
        }
        return *this;
    }

    duration &duration::operator-=(duration rhs) {
        if (chrono_internal::is_infinite_duration(*this)) return *this;
        if (chrono_internal::is_infinite_duration(rhs)) {
            return *this = rhs.rep_hi_ >= 0 ? -infinite_duration() : infinite_duration();
        }
        const int64_t orig_rep_hi = rep_hi_;
        rep_hi_ =
                DecodeTwosComp(EncodeTwosComp(rep_hi_) - EncodeTwosComp(rhs.rep_hi_));
        if (rep_lo_ < rhs.rep_lo_) {
            rep_hi_ = DecodeTwosComp(EncodeTwosComp(rep_hi_) - 1);
            rep_lo_ += kTicksPerSecond;
        }
        rep_lo_ -= rhs.rep_lo_;
        if (rhs.rep_hi_ < 0 ? rep_hi_ < orig_rep_hi : rep_hi_ > orig_rep_hi) {
            return *this = rhs.rep_hi_ >= 0 ? -infinite_duration() : infinite_duration();
        }
        return *this;
    }

//
// Multiplicative operators.
//

    duration &duration::operator*=(int64_t r) {
        if (chrono_internal::is_infinite_duration(*this)) {
            const bool is_neg = (r < 0) != (rep_hi_ < 0);
            return *this = is_neg ? -infinite_duration() : infinite_duration();
        }
        return *this = ScaleFixed<SafeMultiply>(*this, r);
    }

    duration &duration::operator*=(double r) {
        if (chrono_internal::is_infinite_duration(*this) || !IsFinite(r)) {
            const bool is_neg = (std::signbit(r) != 0) != (rep_hi_ < 0);
            return *this = is_neg ? -infinite_duration() : infinite_duration();
        }
        return *this = ScaleDouble<std::multiplies>(*this, r);
    }

    duration &duration::operator/=(int64_t r) {
        if (chrono_internal::is_infinite_duration(*this) || r == 0) {
            const bool is_neg = (r < 0) != (rep_hi_ < 0);
            return *this = is_neg ? -infinite_duration() : infinite_duration();
        }
        return *this = ScaleFixed<std::divides>(*this, r);
    }

    duration &duration::operator/=(double r) {
        if (chrono_internal::is_infinite_duration(*this) || !IsValidDivisor(r)) {
            const bool is_neg = (std::signbit(r) != 0) != (rep_hi_ < 0);
            return *this = is_neg ? -infinite_duration() : infinite_duration();
        }
        return *this = ScaleDouble<std::divides>(*this, r);
    }

    duration &duration::operator%=(duration rhs) {
        chrono_internal::integer_div_duration(false, *this, rhs, this);
        return *this;
    }

    double float_div_duration(duration num, duration den) {
        // Arithmetic with infinity is sticky.
        if (chrono_internal::is_infinite_duration(num) || den == zero_duration()) {
            return (num < zero_duration()) == (den < zero_duration())
                   ? std::numeric_limits<double>::infinity()
                   : -std::numeric_limits<double>::infinity();
        }
        if (chrono_internal::is_infinite_duration(den)) return 0.0;

        double a =
                static_cast<double>(chrono_internal::get_rep_hi(num)) * kTicksPerSecond +
                chrono_internal::get_rep_lo(num);
        double b =
                static_cast<double>(chrono_internal::get_rep_hi(den)) * kTicksPerSecond +
                chrono_internal::get_rep_lo(den);
        return a / b;
    }

//
// trunc/floor/ceil.
//

    duration trunc(duration d, duration unit) {
        return d - (d % unit);
    }

    duration floor(const duration d, const duration unit) {
        const abel::duration td = trunc(d, unit);
        return td <= d ? td : td - abs_duration(unit);
    }

    duration ceil(const duration d, const duration unit) {
        const abel::duration td = trunc(d, unit);
        return td >= d ? td : td + abs_duration(unit);
    }

//
// Factory functions.
//

    duration duration_from_timespec(timespec ts) {
        if (static_cast<uint64_t>(ts.tv_nsec) < 1000 * 1000 * 1000) {
            int64_t ticks = ts.tv_nsec * kTicksPerNanosecond;
            return chrono_internal::make_duration(ts.tv_sec, ticks);
        }
        return seconds(ts.tv_sec) + nanoseconds(ts.tv_nsec);
    }

    duration duration_from_timeval(timeval tv) {
        if (static_cast<uint64_t>(tv.tv_usec) < 1000 * 1000) {
            int64_t ticks = tv.tv_usec * 1000 * kTicksPerNanosecond;
            return chrono_internal::make_duration(tv.tv_sec, ticks);
        }
        return seconds(tv.tv_sec) + microseconds(tv.tv_usec);
    }

//
// Conversion to other duration types.
//

    int64_t to_int64_nanoseconds(duration d) {
        if (chrono_internal::get_rep_hi(d) >= 0 &&
            chrono_internal::get_rep_hi(d) >> 33 == 0) {
            return (chrono_internal::get_rep_hi(d) * 1000 * 1000 * 1000) +
                   (chrono_internal::get_rep_lo(d) / kTicksPerNanosecond);
        }
        return d / nanoseconds(1);
    }

    int64_t to_int64_microseconds(duration d) {
        if (chrono_internal::get_rep_hi(d) >= 0 &&
            chrono_internal::get_rep_hi(d) >> 43 == 0) {
            return (chrono_internal::get_rep_hi(d) * 1000 * 1000) +
                   (chrono_internal::get_rep_lo(d) / (kTicksPerNanosecond * 1000));
        }
        return d / microseconds(1);
    }

    int64_t to_int64_milliseconds(duration d) {
        if (chrono_internal::get_rep_hi(d) >= 0 &&
            chrono_internal::get_rep_hi(d) >> 53 == 0) {
            return (chrono_internal::get_rep_hi(d) * 1000) +
                   (chrono_internal::get_rep_lo(d) / (kTicksPerNanosecond * 1000 * 1000));
        }
        return d / milliseconds(1);
    }

    int64_t to_int64_seconds(duration d) {
        int64_t hi = chrono_internal::get_rep_hi(d);
        if (chrono_internal::is_infinite_duration(d)) return hi;
        if (hi < 0 && chrono_internal::get_rep_lo(d) != 0) ++hi;
        return hi;
    }

    int64_t to_int64_minutes(duration d) {
        int64_t hi = chrono_internal::get_rep_hi(d);
        if (chrono_internal::is_infinite_duration(d)) return hi;
        if (hi < 0 && chrono_internal::get_rep_lo(d) != 0) ++hi;
        return hi / 60;
    }

    int64_t to_int64_hours(duration d) {
        int64_t hi = chrono_internal::get_rep_hi(d);
        if (chrono_internal::is_infinite_duration(d)) return hi;
        if (hi < 0 && chrono_internal::get_rep_lo(d) != 0) ++hi;
        return hi / (60 * 60);
    }

    double to_double_nanoseconds(duration d) {
        return float_div_duration(d, nanoseconds(1));
    }

    double to_double_microseconds(duration d) {
        return float_div_duration(d, microseconds(1));
    }

    double to_double_milliseconds(duration d) {
        return float_div_duration(d, milliseconds(1));
    }

    double to_double_seconds(duration d) {
        return float_div_duration(d, seconds(1));
    }

    double to_double_minutes(duration d) {
        return float_div_duration(d, minutes(1));
    }

    double to_double_hours(duration d) {
        return float_div_duration(d, hours(1));
    }

    timespec to_timespec(duration d) {
        timespec ts;
        if (!chrono_internal::is_infinite_duration(d)) {
            int64_t rep_hi = chrono_internal::get_rep_hi(d);
            uint32_t rep_lo = chrono_internal::get_rep_lo(d);
            if (rep_hi < 0) {
                // Tweak the fields so that unsigned division of rep_lo
                // maps to truncation (towards zero) for the timespec.
                rep_lo += kTicksPerNanosecond - 1;
                if (rep_lo >= kTicksPerSecond) {
                    rep_hi += 1;
                    rep_lo -= kTicksPerSecond;
                }
            }
            ts.tv_sec = rep_hi;
            if (ts.tv_sec == rep_hi) {  // no time_t narrowing
                ts.tv_nsec = rep_lo / kTicksPerNanosecond;
                return ts;
            }
        }
        if (d >= zero_duration()) {
            ts.tv_sec = std::numeric_limits<time_t>::max();
            ts.tv_nsec = 1000 * 1000 * 1000 - 1;
        } else {
            ts.tv_sec = std::numeric_limits<time_t>::min();
            ts.tv_nsec = 0;
        }
        return ts;
    }

    timeval to_timeval(duration d) {
        timeval tv;
        timespec ts = to_timespec(d);
        if (ts.tv_sec < 0) {
            // Tweak the fields so that positive division of tv_nsec
            // maps to truncation (towards zero) for the timeval.
            ts.tv_nsec += 1000 - 1;
            if (ts.tv_nsec >= 1000 * 1000 * 1000) {
                ts.tv_sec += 1;
                ts.tv_nsec -= 1000 * 1000 * 1000;
            }
        }
        tv.tv_sec = ts.tv_sec;
        if (tv.tv_sec != ts.tv_sec) {  // narrowing
            if (ts.tv_sec < 0) {
                tv.tv_sec = std::numeric_limits<decltype(tv.tv_sec)>::min();
                tv.tv_usec = 0;
            } else {
                tv.tv_sec = std::numeric_limits<decltype(tv.tv_sec)>::max();
                tv.tv_usec = 1000 * 1000 - 1;
            }
            return tv;
        }
        tv.tv_usec = static_cast<int>(ts.tv_nsec / 1000);  // suseconds_t
        return tv;
    }

    std::chrono::nanoseconds to_chrono_nanoseconds(duration d) {
        return chrono_internal::to_chrono_duration<std::chrono::nanoseconds>(d);
    }

    std::chrono::microseconds to_chrono_microseconds(duration d) {
        return chrono_internal::to_chrono_duration<std::chrono::microseconds>(d);
    }

    std::chrono::milliseconds to_chrono_milliseconds(duration d) {
        return chrono_internal::to_chrono_duration<std::chrono::milliseconds>(d);
    }

    std::chrono::seconds to_chrono_seconds(duration d) {
        return chrono_internal::to_chrono_duration<std::chrono::seconds>(d);
    }

    std::chrono::minutes to_chrono_minutes(duration d) {
        return chrono_internal::to_chrono_duration<std::chrono::minutes>(d);
    }

    std::chrono::hours to_chrono_hours(duration d) {
        return chrono_internal::to_chrono_duration<std::chrono::hours>(d);
    }

//
// To/From string formatting.
//

    namespace {

// Formats a positive 64-bit integer in the given field width.  Note that
// it is up to the caller of Format64() to ensure that there is sufficient
// space before ep to hold the conversion.
        char *Format64(char *ep, int width, int64_t v) {
            do {
                --width;
                *--ep = '0' + (v % 10);  // contiguous digits
            } while (v /= 10);
            while (--width >= 0) *--ep = '0';  // zero pad
            return ep;
        }

// Helpers for format_duration() that format 'n' and append it to 'out'
// followed by the given 'unit'.  If 'n' formats to "0", nothing is
// appended (not even the unit).

// A type that encapsulates how to display a value of a particular unit. For
// values that are displayed with fractional parts, the precision indicates
// where to round the value. The precision varies with the display unit because
// a duration can hold only quarters of a nanosecond, so displaying information
// beyond that is just noise.
//
// For example, a microsecond value of 42.00025xxxxx should not display beyond 5
// fractional digits, because it is in the noise of what a duration can
// represent.
        struct DisplayUnit {
            const char *abbr;
            int prec;
            double pow10;
        };
        const DisplayUnit kDisplayNano = {"ns", 2, 1e2};
        const DisplayUnit kDisplayMicro = {"us", 5, 1e5};
        const DisplayUnit kDisplayMilli = {"ms", 8, 1e8};
        const DisplayUnit kDisplaySec = {"s", 11, 1e11};
        const DisplayUnit kDisplayMin = {"m", -1, 0.0};   // prec ignored
        const DisplayUnit kDisplayHour = {"h", -1, 0.0};  // prec ignored

        void AppendNumberUnit(std::string *out, int64_t n, DisplayUnit unit) {
            char buf[sizeof("2562047788015216")];  // hours in max duration
            char *const ep = buf + sizeof(buf);
            char *bp = Format64(ep, 0, n);
            if (*bp != '0' || bp + 1 != ep) {
                out->append(bp, ep - bp);
                out->append(unit.abbr);
            }
        }

// Note: unit.prec is limited to double's digits10 value (typically 15) so it
// always fits in buf[].
        void AppendNumberUnit(std::string *out, double n, DisplayUnit unit) {
            const int buf_size = std::numeric_limits<double>::digits10;
            const int prec = std::min(buf_size, unit.prec);
            char buf[buf_size];  // also large enough to hold integer part
            char *ep = buf + sizeof(buf);
            double d = 0;
            int64_t frac_part = Round(std::modf(n, &d) * unit.pow10);
            int64_t int_part = d;
            if (int_part != 0 || frac_part != 0) {
                char *bp = Format64(ep, 0, int_part);  // always < 1000
                out->append(bp, ep - bp);
                if (frac_part != 0) {
                    out->push_back('.');
                    bp = Format64(ep, prec, frac_part);
                    while (ep[-1] == '0') --ep;
                    out->append(bp, ep - bp);
                }
                out->append(unit.abbr);
            }
        }

    }  // namespace

// From Go's doc at https://golang.org/pkg/time/#duration.String
//   [format_duration] returns a string representing the duration in the
//   form "72h3m0.5s". Leading zero units are omitted.  As a special
//   case, durations less than one second format use a smaller unit
//   (milli-, micro-, or nanoseconds) to ensure that the leading digit
//   is non-zero.  The zero duration formats as 0, with no unit.
    std::string format_duration(duration d) {
        const duration min_duration = seconds(kint64min);
        if (d == min_duration) {
            // Avoid needing to negate kint64min by directly returning what the
            // following code should produce in that case.
            return "-2562047788015215h30m8s";
        }
        std::string s;
        if (d < zero_duration()) {
            s.append("-");
            d = -d;
        }
        if (d == infinite_duration()) {
            s.append("inf");
        } else if (d < seconds(1)) {
            // Special case for durations with a magnitude < 1 second.  The duration
            // is printed as a fraction of a single unit, e.g., "1.2ms".
            if (d < microseconds(1)) {
                AppendNumberUnit(&s, float_div_duration(d, nanoseconds(1)), kDisplayNano);
            } else if (d < milliseconds(1)) {
                AppendNumberUnit(&s, float_div_duration(d, microseconds(1)), kDisplayMicro);
            } else {
                AppendNumberUnit(&s, float_div_duration(d, milliseconds(1)), kDisplayMilli);
            }
        } else {
            AppendNumberUnit(&s, integer_div_duration(d, hours(1), &d), kDisplayHour);
            AppendNumberUnit(&s, integer_div_duration(d, minutes(1), &d), kDisplayMin);
            AppendNumberUnit(&s, float_div_duration(d, seconds(1)), kDisplaySec);
        }
        if (s.empty() || s == "-") {
            s = "0";
        }
        return s;
    }

    namespace {

// A helper for parse_duration() that parses a leading number from the given
// string and stores the result in *int_part/*frac_part/*frac_scale.  The
// given string pointer is modified to point to the first unconsumed char.
        bool ConsumeDurationNumber(const char **dpp, int64_t *int_part,
                                   int64_t *frac_part, int64_t *frac_scale) {
            *int_part = 0;
            *frac_part = 0;
            *frac_scale = 1;  // invariant: *frac_part < *frac_scale
            const char *start = *dpp;
            for (; std::isdigit(**dpp); *dpp += 1) {
                const int d = **dpp - '0';  // contiguous digits
                if (*int_part > kint64max / 10) return false;
                *int_part *= 10;
                if (*int_part > kint64max - d) return false;
                *int_part += d;
            }
            const bool int_part_empty = (*dpp == start);
            if (**dpp != '.') return !int_part_empty;
            for (*dpp += 1; std::isdigit(**dpp); *dpp += 1) {
                const int d = **dpp - '0';  // contiguous digits
                if (*frac_scale <= kint64max / 10) {
                    *frac_part *= 10;
                    *frac_part += d;
                    *frac_scale *= 10;
                }
            }
            return !int_part_empty || *frac_scale != 1;
        }

// A helper for parse_duration() that parses a leading unit designator (e.g.,
// ns, us, ms, s, m, h) from the given string and stores the resulting unit
// in "*unit".  The given string pointer is modified to point to the first
// unconsumed char.
        bool ConsumeDurationUnit(const char **start, duration *unit) {
            const char *s = *start;
            bool ok = true;
            if (strncmp(s, "ns", 2) == 0) {
                s += 2;
                *unit = nanoseconds(1);
            } else if (strncmp(s, "us", 2) == 0) {
                s += 2;
                *unit = microseconds(1);
            } else if (strncmp(s, "ms", 2) == 0) {
                s += 2;
                *unit = milliseconds(1);
            } else if (strncmp(s, "s", 1) == 0) {
                s += 1;
                *unit = seconds(1);
            } else if (strncmp(s, "m", 1) == 0) {
                s += 1;
                *unit = minutes(1);
            } else if (strncmp(s, "h", 1) == 0) {
                s += 1;
                *unit = hours(1);
            } else {
                ok = false;
            }
            *start = s;
            return ok;
        }

    }  // namespace

// From Go's doc at https://golang.org/pkg/time/#parse_duration
//   [parse_duration] parses a duration string. A duration string is
//   a possibly signed sequence of decimal numbers, each with optional
//   fraction and a unit suffix, such as "300ms", "-1.5h" or "2h45m".
//   Valid time units are "ns", "us" "ms", "s", "m", "h".
    bool parse_duration(const std::string &dur_string, duration *d) {
        const char *start = dur_string.c_str();
        int sign = 1;

        if (*start == '-' || *start == '+') {
            sign = *start == '-' ? -1 : 1;
            ++start;
        }

        // Can't parse a duration from an empty std::string.
        if (*start == '\0') {
            return false;
        }

        // Special case for a std::string of "0".
        if (*start == '0' && *(start + 1) == '\0') {
            *d = zero_duration();
            return true;
        }

        if (strcmp(start, "inf") == 0) {
            *d = sign * infinite_duration();
            return true;
        }

        duration dur;
        while (*start != '\0') {
            int64_t int_part;
            int64_t frac_part;
            int64_t frac_scale;
            duration unit;
            if (!ConsumeDurationNumber(&start, &int_part, &frac_part, &frac_scale) ||
                !ConsumeDurationUnit(&start, &unit)) {
                return false;
            }
            if (int_part != 0) dur += sign * int_part * unit;
            if (frac_part != 0) dur += sign * frac_part * unit / frac_scale;
        }
        *d = dur;
        return true;
    }

    bool abel_parse_flag(abel::string_view text, duration *dst, std::string *) {
        return parse_duration(std::string(text), dst);
    }

    std::string abel_unparse_flag(duration d) { return format_duration(d); }

    bool ParseFlag(const std::string &text, duration *dst, std::string *) {
        return parse_duration(text, dst);
    }

    std::string UnparseFlag(duration d) { return format_duration(d); }


}  // namespace abel
