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
// ci: An abel::time_zone::CivilInfo
// ti: An abel::time_zone::TimeInfo
// cd: An abel::CivilDay or a cctz::civil_day
// cs: An abel::CivilSecond or a cctz::civil_second
// bd: An abel::abel_time::breakdown
// cl: A cctz::time_zone::civil_lookup
// al: A cctz::time_zone::absolute_lookup

#include <abel/time/time.h>

#if defined(_MSC_VER)
#include <winsock2.h>  // for timeval
#endif

#include <cstring>
#include <ctime>
#include <limits>

#include <abel/time/internal/cctz/include/cctz/civil_time.h>
#include <abel/time/internal/cctz/include/cctz/time_zone.h>

namespace cctz = abel::time_internal::cctz;

namespace abel {
ABEL_NAMESPACE_BEGIN

namespace {

ABEL_FORCE_INLINE cctz::time_point<cctz::seconds> internal_unix_epoch() {
  return std::chrono::time_point_cast<cctz::seconds>(
      std::chrono::system_clock::from_time_t(0));
}

// Floors d to the next unit boundary closer to negative infinity.
ABEL_FORCE_INLINE int64_t FloorToUnit(abel::duration d, abel::duration unit) {
  abel::duration rem;
  int64_t q = abel::integer_div_duration(d, unit, &rem);
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

ABEL_FORCE_INLINE abel::time_zone::CivilInfo InfiniteFutureCivilInfo() {
  time_zone::CivilInfo ci;
  ci.cs = CivilSecond::max();
  ci.subsecond = infinite_duration();
  ci.offset = 0;
  ci.is_dst = false;
  ci.zone_abbr = "-00";
  return ci;
}

ABEL_FORCE_INLINE abel::time_zone::CivilInfo InfinitePastCivilInfo() {
  time_zone::CivilInfo ci;
  ci.cs = CivilSecond::min();
  ci.subsecond = -infinite_duration();
  ci.offset = 0;
  ci.is_dst = false;
  ci.zone_abbr = "-00";
  return ci;
}

ABEL_FORCE_INLINE abel::TimeConversion InfiniteFutureTimeConversion() {
  abel::TimeConversion tc;
  tc.pre = tc.trans = tc.post = abel::infinite_future();
  tc.kind = abel::TimeConversion::UNIQUE;
  tc.normalized = true;
  return tc;
}

ABEL_FORCE_INLINE TimeConversion InfinitePastTimeConversion() {
  abel::TimeConversion tc;
  tc.pre = tc.trans = tc.post = abel::infinite_past();
  tc.kind = abel::TimeConversion::UNIQUE;
  tc.normalized = true;
  return tc;
}

// Makes a abel_time from sec, overflowing to infinite_future/infinite_past as
// necessary. If sec is min/max, then consult cs+tz to check for overlow.
abel_time MakeTimeWithOverflow(const cctz::time_point<cctz::seconds>& sec,
                          const cctz::civil_second& cs,
                          const cctz::time_zone& tz,
                          bool* normalized = nullptr) {
  const auto max = cctz::time_point<cctz::seconds>::max();
  const auto min = cctz::time_point<cctz::seconds>::min();
  if (sec == max) {
    const auto al = tz.lookup(max);
    if (cs > al.cs) {
      if (normalized) *normalized = true;
      return abel::infinite_future();
    }
  }
  if (sec == min) {
    const auto al = tz.lookup(min);
    if (cs < al.cs) {
      if (normalized) *normalized = true;
      return abel::infinite_past();
    }
  }
  const auto hi = (sec - internal_unix_epoch()).count();
  return time_internal::from_unix_duration(time_internal::MakeDuration(hi));
}

// Returns Mon=1..Sun=7.
ABEL_FORCE_INLINE int MapWeekday(const cctz::weekday& wd) {
  switch (wd) {
    case cctz::weekday::monday:
      return 1;
    case cctz::weekday::tuesday:
      return 2;
    case cctz::weekday::wednesday:
      return 3;
    case cctz::weekday::thursday:
      return 4;
    case cctz::weekday::friday:
      return 5;
    case cctz::weekday::saturday:
      return 6;
    case cctz::weekday::sunday:
      return 7;
  }
  return 1;
}

bool FindTransition(const cctz::time_zone& tz,
                    bool (cctz::time_zone::*find_transition)(
                        const cctz::time_point<cctz::seconds>& tp,
                        cctz::time_zone::civil_transition* trans) const,
                    abel_time t, time_zone::CivilTransition* trans) {
  // Transitions are second-aligned, so we can discard any fractional part.
  const auto tp = internal_unix_epoch() + cctz::seconds(ToUnixSeconds(t));
  cctz::time_zone::civil_transition tr;
  if (!(tz.*find_transition)(tp, &tr)) return false;
  trans->from = CivilSecond(tr.from);
  trans->to = CivilSecond(tr.to);
  return true;
}

}  // namespace

//
// abel_time
//

abel::abel_time::breakdown abel_time::in(abel::time_zone tz) const {
  if (*this == abel::infinite_future()) return InfiniteFutureBreakdown();
  if (*this == abel::infinite_past()) return InfinitePastBreakdown();

  const auto tp = internal_unix_epoch() + cctz::seconds(time_internal::GetRepHi(rep_));
  const auto al = cctz::time_zone(tz).lookup(tp);
  const auto cs = al.cs;
  const auto cd = cctz::civil_day(cs);

  abel::abel_time::breakdown bd;
  bd.year = cs.year();
  bd.month = cs.month();
  bd.day = cs.day();
  bd.hour = cs.hour();
  bd.minute = cs.minute();
  bd.second = cs.second();
  bd.subsecond = time_internal::MakeDuration(0, time_internal::GetRepLo(rep_));
  bd.weekday = MapWeekday(cctz::get_weekday(cd));
  bd.yearday = cctz::get_yearday(cd);
  bd.offset = al.offset;
  bd.is_dst = al.is_dst;
  bd.zone_abbr = al.abbr;
  return bd;
}

//
// Conversions from/to other time types.
//

abel::abel_time from_date(double udate) {
  return time_internal::from_unix_duration(abel::milliseconds(udate));
}

abel::abel_time from_universal(int64_t universal) {
  return abel::universal_epoch() + 100 * abel::nanoseconds(universal);
}

int64_t to_unix_nanos(abel_time t) {
  if (time_internal::GetRepHi(time_internal::to_unix_duration(t)) >= 0 &&
      time_internal::GetRepHi(time_internal::to_unix_duration(t)) >> 33 == 0) {
    return (time_internal::GetRepHi(time_internal::to_unix_duration(t)) *
            1000 * 1000 * 1000) +
           (time_internal::GetRepLo(time_internal::to_unix_duration(t)) / 4);
  }
  return FloorToUnit(time_internal::to_unix_duration(t), abel::nanoseconds(1));
}

int64_t to_unix_micros(abel_time t) {
  if (time_internal::GetRepHi(time_internal::to_unix_duration(t)) >= 0 &&
      time_internal::GetRepHi(time_internal::to_unix_duration(t)) >> 43 == 0) {
    return (time_internal::GetRepHi(time_internal::to_unix_duration(t)) *
            1000 * 1000) +
           (time_internal::GetRepLo(time_internal::to_unix_duration(t)) / 4000);
  }
  return FloorToUnit(time_internal::to_unix_duration(t), abel::microseconds(1));
}

int64_t to_unix_millis(abel_time t) {
  if (time_internal::GetRepHi(time_internal::to_unix_duration(t)) >= 0 &&
      time_internal::GetRepHi(time_internal::to_unix_duration(t)) >> 53 == 0) {
    return (time_internal::GetRepHi(time_internal::to_unix_duration(t)) * 1000) +
           (time_internal::GetRepLo(time_internal::to_unix_duration(t)) /
            (4000 * 1000));
  }
  return FloorToUnit(time_internal::to_unix_duration(t), abel::milliseconds(1));
}

int64_t ToUnixSeconds(abel_time t) {
  return time_internal::GetRepHi(time_internal::to_unix_duration(t));
}

time_t to_time_t(abel_time t) { return abel::to_timespec(t).tv_sec; }

double to_date(abel_time t) {
  return abel::float_div_duration(time_internal::to_unix_duration(t),
                            abel::milliseconds(1));
}

int64_t to_universal(abel::abel_time t) {
  return abel::FloorToUnit(t - abel::universal_epoch(), abel::nanoseconds(100));
}

abel::abel_time time_from_timespec(timespec ts) {
  return time_internal::from_unix_duration(abel::duration_from_timespec(ts));
}

abel::abel_time time_from_timeval(timeval tv) {
  return time_internal::from_unix_duration(abel::duration_from_timeval(tv));
}

timespec to_timespec(abel_time t) {
  timespec ts;
  abel::duration d = time_internal::to_unix_duration(t);
  if (!time_internal::IsInfiniteDuration(d)) {
    ts.tv_sec = time_internal::GetRepHi(d);
    if (ts.tv_sec == time_internal::GetRepHi(d)) {  // no time_t narrowing
      ts.tv_nsec = time_internal::GetRepLo(d) / 4;  // floor
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

timeval to_timeval(abel_time t) {
  timeval tv;
  timespec ts = abel::to_timespec(t);
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

abel_time from_chrono(const std::chrono::system_clock::time_point& tp) {
  return time_internal::from_unix_duration(time_internal::from_chrono(
      tp - std::chrono::system_clock::from_time_t(0)));
}

std::chrono::system_clock::time_point to_chrono_time(abel::abel_time t) {
  using D = std::chrono::system_clock::duration;
  auto d = time_internal::to_unix_duration(t);
  if (d < zero_duration()) d = floor(d, from_chrono(D{1}));
  return std::chrono::system_clock::from_time_t(0) +
         time_internal::ToChronoDuration<D>(d);
}

//
// time_zone
//

abel::time_zone::CivilInfo time_zone::At(abel_time t) const {
  if (t == abel::infinite_future()) return InfiniteFutureCivilInfo();
  if (t == abel::infinite_past()) return InfinitePastCivilInfo();

  const auto ud = time_internal::to_unix_duration(t);
  const auto tp = internal_unix_epoch() + cctz::seconds(time_internal::GetRepHi(ud));
  const auto al = cz_.lookup(tp);

  time_zone::CivilInfo ci;
  ci.cs = CivilSecond(al.cs);
  ci.subsecond = time_internal::MakeDuration(0, time_internal::GetRepLo(ud));
  ci.offset = al.offset;
  ci.is_dst = al.is_dst;
  ci.zone_abbr = al.abbr;
  return ci;
}

abel::time_zone::TimeInfo time_zone::At(CivilSecond ct) const {
  const cctz::civil_second cs(ct);
  const auto cl = cz_.lookup(cs);

  time_zone::TimeInfo ti;
  switch (cl.kind) {
    case cctz::time_zone::civil_lookup::UNIQUE:
      ti.kind = time_zone::TimeInfo::UNIQUE;
      break;
    case cctz::time_zone::civil_lookup::SKIPPED:
      ti.kind = time_zone::TimeInfo::SKIPPED;
      break;
    case cctz::time_zone::civil_lookup::REPEATED:
      ti.kind = time_zone::TimeInfo::REPEATED;
      break;
  }
  ti.pre = MakeTimeWithOverflow(cl.pre, cs, cz_);
  ti.trans = MakeTimeWithOverflow(cl.trans, cs, cz_);
  ti.post = MakeTimeWithOverflow(cl.post, cs, cz_);
  return ti;
}

bool time_zone::NextTransition(abel_time t, CivilTransition* trans) const {
  return FindTransition(cz_, &cctz::time_zone::next_transition, t, trans);
}

bool time_zone::PrevTransition(abel_time t, CivilTransition* trans) const {
  return FindTransition(cz_, &cctz::time_zone::prev_transition, t, trans);
}

//
// Conversions involving time zones.
//

abel::TimeConversion convert_date_time(int64_t year, int mon, int day, int hour,
                                     int min, int sec, time_zone tz) {
  // Avoids years that are too extreme for CivilSecond to normalize.
  if (year > 300000000000) return InfiniteFutureTimeConversion();
  if (year < -300000000000) return InfinitePastTimeConversion();

  const CivilSecond cs(year, mon, day, hour, min, sec);
  const auto ti = tz.At(cs);

  TimeConversion tc;
  tc.pre = ti.pre;
  tc.trans = ti.trans;
  tc.post = ti.post;
  switch (ti.kind) {
    case time_zone::TimeInfo::UNIQUE:
      tc.kind = TimeConversion::UNIQUE;
      break;
    case time_zone::TimeInfo::SKIPPED:
      tc.kind = TimeConversion::SKIPPED;
      break;
    case time_zone::TimeInfo::REPEATED:
      tc.kind = TimeConversion::REPEATED;
      break;
  }
  tc.normalized = false;
  if (year != cs.year() || mon != cs.month() || day != cs.day() ||
      hour != cs.hour() || min != cs.minute() || sec != cs.second()) {
    tc.normalized = true;
  }
  return tc;
}

abel::abel_time FromTM(const struct tm& tm, abel::time_zone tz) {
  civil_year_t tm_year = tm.tm_year;
  // Avoids years that are too extreme for CivilSecond to normalize.
  if (tm_year > 300000000000ll) return infinite_future();
  if (tm_year < -300000000000ll) return infinite_past();
  int tm_mon = tm.tm_mon;
  if (tm_mon == std::numeric_limits<int>::max()) {
    tm_mon -= 12;
    tm_year += 1;
  }
  const auto ti = tz.At(CivilSecond(tm_year + 1900, tm_mon + 1, tm.tm_mday,
                                    tm.tm_hour, tm.tm_min, tm.tm_sec));
  return tm.tm_isdst == 0 ? ti.post : ti.pre;
}

struct tm ToTM(abel::abel_time t, abel::time_zone tz) {
  struct tm tm = {};

  const auto ci = tz.At(t);
  const auto& cs = ci.cs;
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
    case Weekday::sunday:
      tm.tm_wday = 0;
      break;
    case Weekday::monday:
      tm.tm_wday = 1;
      break;
    case Weekday::tuesday:
      tm.tm_wday = 2;
      break;
    case Weekday::wednesday:
      tm.tm_wday = 3;
      break;
    case Weekday::thursday:
      tm.tm_wday = 4;
      break;
    case Weekday::friday:
      tm.tm_wday = 5;
      break;
    case Weekday::saturday:
      tm.tm_wday = 6;
      break;
  }
  tm.tm_yday = GetYearDay(cs) - 1;
  tm.tm_isdst = ci.is_dst ? 1 : 0;

  return tm;
}

ABEL_NAMESPACE_END
}  // namespace abel
