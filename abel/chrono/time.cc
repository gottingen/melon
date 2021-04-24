// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com

//

// The implementation of the abel::abel_time class, which is declared in
// //abel/time.h.
//
// The representation for an abel::abel_time is an abel::duration offset from the
// epoch.  We use the traditional Unix epoch (1970-01-01 00:00:00 +0000)
// for convenience, but this is not exposed in the API and could be changed.
//
// NOTE: To keep type verbosity to a minimum, the following variable naming
// conventions are used throughout this file.
//
// tz: An abel::time_zone
// ci: An abel::time_zone::chrono_info
// ti: An abel::time_zone::time_info
// cd: An abel::chrono_day or a abel::chrono_internal::civil_day
// cs: An abel::chrono_second or a abel::chrono_internal::civil_second
// bd: An abel::abel_time::breakdown
// cl: A abel::chrono_internal::time_zone::civil_lookup
// al: A abel::chrono_internal::time_zone::absolute_lookup

#include "abel/chrono/time.h"

#if defined(_MSC_VER)
#include <winsock2.h>  // for timeval
#endif

#include <cstring>
#include <ctime>
#include <limits>
#include "abel/chrono/internal/time_zone.h"

namespace abel {

namespace {

ABEL_FORCE_INLINE abel::chrono_internal::time_point<abel::chrono_internal::seconds> internal_unix_epoch() {
    return std::chrono::time_point_cast<abel::chrono_internal::seconds>(
            std::chrono::system_clock::from_time_t(0));
}

// Floors d to the next unit boundary closer to negative infinity.
ABEL_FORCE_INLINE int64_t FloorToUnit(abel::duration d, abel::duration unit) {
    abel::duration rem;
    int64_t q = duration::integer_div_duration(d, unit, &rem);
    return (q > 0 ||
            rem >= zero_duration() ||
            q == std::numeric_limits<int64_t>::min()) ? q : q - 1;
}

ABEL_FORCE_INLINE abel::abel_time::breakdown InfiniteFutureBreakdown() {
    abel::abel_time::breakdown bd;
    bd.year = std::numeric_limits<int64_t>::max();
    bd.month = 12;
    bd.day = 31;
    bd.hour = 23;
    bd.minute = 59;
    bd.second = 59;
    bd.subsecond = abel::infinite_duration();
    bd.weekday = 4;
    bd.yearday = 365;
    bd.offset = 0;
    bd.is_dst = false;
    bd.zone_abbr = "-00";
    return bd;
}

ABEL_FORCE_INLINE abel::abel_time::breakdown InfinitePastBreakdown() {
    abel_time::breakdown bd;
    bd.year = std::numeric_limits<int64_t>::min();
    bd.month = 1;
    bd.day = 1;
    bd.hour = 0;
    bd.minute = 0;
    bd.second = 0;
    bd.subsecond = -abel::infinite_duration();
    bd.weekday = 7;
    bd.yearday = 1;
    bd.offset = 0;
    bd.is_dst = false;
    bd.zone_abbr = "-00";
    return bd;
}

ABEL_FORCE_INLINE abel::time_zone::chrono_info InfiniteFutureCivilInfo() {
    time_zone::chrono_info ci;
    ci.cs = chrono_second::max();
    ci.subsecond = infinite_duration();
    ci.offset = 0;
    ci.is_dst = false;
    ci.zone_abbr = "-00";
    return ci;
}

ABEL_FORCE_INLINE abel::time_zone::chrono_info InfinitePastCivilInfo() {
    time_zone::chrono_info ci;
    ci.cs = chrono_second::min();
    ci.subsecond = -infinite_duration();
    ci.offset = 0;
    ci.is_dst = false;
    ci.zone_abbr = "-00";
    return ci;
}

ABEL_FORCE_INLINE abel::time_conversion InfiniteFutureTimeConversion() {
    abel::time_conversion tc;
    tc.pre = tc.trans = tc.post = abel::abel_time::infinite_future();
    tc.kind = abel::time_conversion::UNIQUE;
    tc.normalized = true;
    return tc;
}

ABEL_FORCE_INLINE time_conversion InfinitePastTimeConversion() {
    abel::time_conversion tc;
    tc.pre = tc.trans = tc.post = abel::abel_time::infinite_past();
    tc.kind = abel::time_conversion::UNIQUE;
    tc.normalized = true;
    return tc;
}

// Makes a abel_time from sec, overflowing to infinite_future/infinite_past as
// necessary. If sec is min/max, then consult cs+tz to check for overlow.
abel_time MakeTimeWithOverflow(const abel::chrono_internal::time_point<abel::chrono_internal::seconds> &sec,
                               const abel::chrono_internal::civil_second &cs,
                               const abel::chrono_internal::time_zone &tz,
                               bool *normalized = nullptr) {
    const auto max = abel::chrono_internal::time_point<abel::chrono_internal::seconds>::max();
    const auto min = abel::chrono_internal::time_point<abel::chrono_internal::seconds>::min();
    if (sec == max) {
        const auto al = tz.lookup(max);
        if (cs > al.cs) {
            if (normalized)
                *normalized = true;
            return abel::abel_time::infinite_future();
        }
    }
    if (sec == min) {
        const auto al = tz.lookup(min);
        if (cs < al.cs) {
            if (normalized)
                *normalized = true;
            return abel::abel_time::infinite_past();
        }
    }
    const auto hi = (sec - internal_unix_epoch()).count();
    return abel_time::from_unix_duration(duration::make_duration(hi));
}

// Returns Mon=1..Sun=7.
ABEL_FORCE_INLINE int MapWeekday(const abel::chrono_internal::weekday &wd) {
    switch (wd) {
        case abel::chrono_internal::weekday::monday:
            return 1;
        case abel::chrono_internal::weekday::tuesday:
            return 2;
        case abel::chrono_internal::weekday::wednesday:
            return 3;
        case abel::chrono_internal::weekday::thursday:
            return 4;
        case abel::chrono_internal::weekday::friday:
            return 5;
        case abel::chrono_internal::weekday::saturday:
            return 6;
        case abel::chrono_internal::weekday::sunday:
            return 7;
    }
    return 1;
}

bool FindTransition(const abel::chrono_internal::time_zone &tz,
                    bool (abel::chrono_internal::time_zone::*find_transition)(
                            const abel::chrono_internal::time_point<abel::chrono_internal::seconds> &tp,
                            abel::chrono_internal::time_zone::civil_transition *trans) const,
                    abel_time t, time_zone::chrono_transition *trans) {
    // Transitions are second-aligned, so we can discard any fractional part.
    const auto tp = internal_unix_epoch() + abel::chrono_internal::seconds(t.to_unix_seconds());
    abel::chrono_internal::time_zone::civil_transition tr;
    if (!(tz.*find_transition)(tp, &tr))
        return false;
    trans->from = chrono_second(tr.from);
    trans->to = chrono_second(tr.to);
    return true;
}

}  // namespace

//
// abel_time
//

abel::abel_time::breakdown abel_time::in(abel::time_zone tz) const {
    if (*this == abel::abel_time::infinite_future())
        return InfiniteFutureBreakdown();
    if (*this == abel::abel_time::infinite_past())
        return InfinitePastBreakdown();

    const auto tp = internal_unix_epoch() + abel::chrono_internal::seconds(duration::get_rep_hi(rep_));
    const auto al = abel::chrono_internal::time_zone(tz).lookup(tp);
    const auto cs = al.cs;
    const auto cd = abel::chrono_internal::civil_day(cs);

    abel::abel_time::breakdown bd;
    bd.year = cs.year();
    bd.month = cs.month();
    bd.day = cs.day();
    bd.hour = cs.hour();
    bd.minute = cs.minute();
    bd.second = cs.second();
    bd.subsecond = duration::make_duration(0, duration::get_rep_lo(rep_));
    bd.weekday = MapWeekday(abel::chrono_internal::get_weekday(cd));
    bd.yearday = abel::chrono_internal::get_yearday(cd);
    bd.offset = al.offset;
    bd.is_dst = al.is_dst;
    bd.zone_abbr = al.abbr;
    return bd;
}

//
// Conversions from/to other time types.
//

abel::abel_time abel_time::from_date(double udate) {
    return abel_time::from_unix_duration(duration::milliseconds(udate));
}

abel::abel_time abel_time::from_universal(int64_t universal) {
    return abel::abel_time::universal_epoch() + 100 * duration::nanoseconds(universal);
}

int64_t abel_time::to_unix_nanos() const {
    if (duration::get_rep_hi(abel_time::to_unix_duration(*this)) >= 0 &&
        duration::get_rep_hi(abel_time::to_unix_duration(*this)) >> 33 == 0) {
        return (duration::get_rep_hi(abel_time::to_unix_duration(*this)) *
                1000 * 1000 * 1000) +
               (duration::get_rep_lo(abel_time::to_unix_duration(*this)) / 4);
    }
    return FloorToUnit(abel_time::to_unix_duration(*this), duration::nanoseconds(1));
}

int64_t abel_time::to_unix_micros() const {
    if (duration::get_rep_hi(abel_time::to_unix_duration(*this)) >= 0 &&
        duration::get_rep_hi(abel_time::to_unix_duration(*this)) >> 43 == 0) {
        return (duration::get_rep_hi(abel_time::to_unix_duration(*this)) *
                1000 * 1000) +
               (duration::get_rep_lo(abel_time::to_unix_duration(*this)) / 4000);
    }
    return FloorToUnit(abel_time::to_unix_duration(*this), duration::microseconds(1));
}

int64_t abel_time::to_unix_millis() const {
    if (duration::get_rep_hi(abel_time::to_unix_duration(*this)) >= 0 &&
        duration::get_rep_hi(abel_time::to_unix_duration(*this)) >> 53 == 0) {
        return (duration::get_rep_hi(abel_time::to_unix_duration(*this)) * 1000) +
               (duration::get_rep_lo(abel_time::to_unix_duration(*this)) /
                (4000 * 1000));
    }
    return FloorToUnit(abel_time::to_unix_duration(*this), duration::milliseconds(1));
}

int64_t abel_time::to_unix_seconds() const {
    return duration::get_rep_hi(abel_time::to_unix_duration(*this));
}

time_t abel_time::to_time_t() const { return to_timespec().tv_sec; }

double abel_time::to_date() const {
    return rep_.float_div_duration(duration::milliseconds(1));
}

int64_t abel_time::to_universal() const {
    return abel::FloorToUnit(*this - abel::abel_time::universal_epoch(), duration::nanoseconds(100));
}

abel::abel_time abel_time::from_timespec(timespec ts) {
    return from_unix_duration(duration::from_timespec(ts));
}

abel::abel_time abel_time::from_timeval(timeval tv) {
    return from_unix_duration(duration::from_timeval(tv));
}

timespec abel_time::to_timespec() const {
    timespec ts;
    abel::duration d = to_unix_duration(*this);
    if (!d.is_infinite_duration()) {
        ts.tv_sec = duration::get_rep_hi(d);
        if (ts.tv_sec == duration::get_rep_hi(d)) {  // no time_t narrowing
            ts.tv_nsec = duration::get_rep_lo(d) / 4;  // floor
            return ts;
        }
    }
    if (d >= abel::zero_duration()) {
        ts.tv_sec = std::numeric_limits<time_t>::max();
        ts.tv_nsec = 1000 * 1000 * 1000 - 1;
    } else {
        ts.tv_sec = std::numeric_limits<time_t>::min();
        ts.tv_nsec = 0;
    }
    return ts;
}

timeval abel_time::to_timeval() const {
    timeval tv;
    timespec ts = to_timespec();
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

abel_time abel_time::from_chrono(const std::chrono::system_clock::time_point &tp) {
    return abel_time::from_unix_duration(duration::from_chrono(tp - std::chrono::system_clock::from_time_t(0)));
}

std::chrono::system_clock::time_point abel_time::to_chrono_time() const {
    using D = std::chrono::system_clock::duration;
    auto d = abel_time::to_unix_duration(*this);
    if (d < zero_duration())
        d = d.floor(duration::from_chrono(D{1}));
    return std::chrono::system_clock::from_time_t(0) +
           d.to_chrono_duration<D>();
}

//
// time_zone
//

abel::time_zone::chrono_info time_zone::at(abel_time t) const {
    if (t == abel::abel_time::infinite_future())
        return InfiniteFutureCivilInfo();
    if (t == abel::abel_time::infinite_past())
        return InfinitePastCivilInfo();

    const auto ud = abel_time::to_unix_duration(t);
    const auto tp = internal_unix_epoch() + abel::chrono_internal::seconds(duration::get_rep_hi(ud));
    const auto al = cz_.lookup(tp);

    time_zone::chrono_info ci;
    ci.cs = chrono_second(al.cs);
    ci.subsecond = duration::make_duration(0, duration::get_rep_lo(ud));
    ci.offset = al.offset;
    ci.is_dst = al.is_dst;
    ci.zone_abbr = al.abbr;
    return ci;
}

abel::time_zone::time_info time_zone::at(chrono_second ct) const {
    const abel::chrono_internal::civil_second cs(ct);
    const auto cl = cz_.lookup(cs);

    time_zone::time_info ti;
    switch (cl.kind) {
        case abel::chrono_internal::time_zone::civil_lookup::UNIQUE:
            ti.kind = time_zone::time_info::UNIQUE;
            break;
        case abel::chrono_internal::time_zone::civil_lookup::SKIPPED:
            ti.kind = time_zone::time_info::SKIPPED;
            break;
        case abel::chrono_internal::time_zone::civil_lookup::REPEATED:
            ti.kind = time_zone::time_info::REPEATED;
            break;
    }
    ti.pre = MakeTimeWithOverflow(cl.pre, cs, cz_);
    ti.trans = MakeTimeWithOverflow(cl.trans, cs, cz_);
    ti.post = MakeTimeWithOverflow(cl.post, cs, cz_);
    return ti;
}

bool time_zone::next_transition(abel_time t, chrono_transition *trans) const {
    return FindTransition(cz_, &abel::chrono_internal::time_zone::next_transition, t, trans);
}

bool time_zone::prev_transition(abel_time t, chrono_transition *trans) const {
    return FindTransition(cz_, &abel::chrono_internal::time_zone::prev_transition, t, trans);
}

//
// Conversions involving time zones.
//

abel::time_conversion convert_date_time(int64_t year, int mon, int day, int hour,
                                        int min, int sec, time_zone tz) {
    // Avoids years that are too extreme for chrono_second to normalize.
    if (year > 300000000000)
        return InfiniteFutureTimeConversion();
    if (year < -300000000000)
        return InfinitePastTimeConversion();

    const chrono_second cs(year, mon, day, hour, min, sec);
    const auto ti = tz.at(cs);

    time_conversion tc;
    tc.pre = ti.pre;
    tc.trans = ti.trans;
    tc.post = ti.post;
    switch (ti.kind) {
        case time_zone::time_info::UNIQUE:
            tc.kind = time_conversion::UNIQUE;
            break;
        case time_zone::time_info::SKIPPED:
            tc.kind = time_conversion::SKIPPED;
            break;
        case time_zone::time_info::REPEATED:
            tc.kind = time_conversion::REPEATED;
            break;
    }
    tc.normalized = false;
    if (year != cs.year() || mon != cs.month() || day != cs.day() ||
        hour != cs.hour() || min != cs.minute() || sec != cs.second()) {
        tc.normalized = true;
    }
    return tc;
}

abel::abel_time from_tm(const struct tm &tm, abel::time_zone tz) {
    chrono_year_t tm_year = tm.tm_year;
    // Avoids years that are too extreme for chrono_second to normalize.
    if (tm_year > 300000000000ll)
        return abel_time::infinite_future();
    if (tm_year < -300000000000ll)
        return abel_time::infinite_past();
    int tm_mon = tm.tm_mon;
    if (tm_mon == std::numeric_limits<int>::max()) {
        tm_mon -= 12;
        tm_year += 1;
    }
    const auto ti = tz.at(chrono_second(tm_year + 1900, tm_mon + 1, tm.tm_mday,
                                        tm.tm_hour, tm.tm_min, tm.tm_sec));
    return tm.tm_isdst == 0 ? ti.post : ti.pre;
}

struct tm to_tm(abel::abel_time t, abel::time_zone tz) {
    struct tm tm = {};

    const auto ci = tz.at(t);
    const auto &cs = ci.cs;
    tm.tm_sec = cs.second();
    tm.tm_min = cs.minute();
    tm.tm_hour = cs.hour();
    tm.tm_mday = cs.day();
    tm.tm_mon = cs.month() - 1;

    // Saturates tm.tm_year in cases of over/underflow, accounting for the fact
    // that tm.tm_year is years since 1900.
    if (cs.year() < std::numeric_limits<int>::min() + 1900) {
        tm.tm_year = std::numeric_limits<int>::min();
    } else if (cs.year() > std::numeric_limits<int>::max()) {
        tm.tm_year = std::numeric_limits<int>::max() - 1900;
    } else {
        tm.tm_year = static_cast<int>(cs.year() - 1900);
    }

    switch (GetWeekday(cs)) {
        case chrono_weekday::sunday:
            tm.tm_wday = 0;
            break;
        case chrono_weekday::monday:
            tm.tm_wday = 1;
            break;
        case chrono_weekday::tuesday:
            tm.tm_wday = 2;
            break;
        case chrono_weekday::wednesday:
            tm.tm_wday = 3;
            break;
        case chrono_weekday::thursday:
            tm.tm_wday = 4;
            break;
        case chrono_weekday::friday:
            tm.tm_wday = 5;
            break;
        case chrono_weekday::saturday:
            tm.tm_wday = 6;
            break;
    }
    tm.tm_yday = get_yearday(cs) - 1;
    tm.tm_isdst = ci.is_dst ? 1 : 0;

    return tm;
}

}  // namespace abel
