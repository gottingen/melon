//
// Created by liyinbin on 2020/1/28.
//

#ifndef ABEL_CHRONO_INTERNAL_CHRONO_TIME_DETAIL_H_
#define ABEL_CHRONO_INTERNAL_CHRONO_TIME_DETAIL_H_

#include <cstdint>
#include <limits>
#include <ostream>
#include <type_traits>

#include <abel/base/profile.h>

namespace abel {
    namespace chrono_internal {



// Support yearsthat at least span the range of 64-bit time_t values.
        using year_t = std::int_fast64_t;

// Type alias that indicates an argument is not normalized (e.g., the
// constructor parameters and operands/results of addition/subtraction).
        using diff_t = std::int_fast64_t;

        namespace detail {

// Type aliases that indicate normalized argument values.
            using month_t = std::int_fast8_t;   // [1:12]
            using day_t = std::int_fast8_t;     // [1:31]
            using hour_t = std::int_fast8_t;    // [0:23]
            using minute_t = std::int_fast8_t;  // [0:59]
            using second_t = std::int_fast8_t;  // [0:59]

// Normalized civil-time fields: Y-M-D HH:MM:SS.
            struct fields {
                ABEL_CONSTEXPR_MEMBER fields(year_t year, month_t month, day_t day, hour_t hour,
                                             minute_t minute, second_t second)
                        : y(year), m(month), d(day), hh(hour), mm(minute), ss(second) {}

                std::int_least64_t y;
                std::int_least8_t m;
                std::int_least8_t d;
                std::int_least8_t hh;
                std::int_least8_t mm;
                std::int_least8_t ss;
            };

            struct second_tag {
            };
            struct minute_tag : second_tag {
            };
            struct hour_tag : minute_tag {
            };
            struct day_tag : hour_tag {
            };
            struct month_tag : day_tag {
            };
            struct year_tag : month_tag {
            };

////////////////////////////////////////////////////////////////////////

// Field normalization (without avoidable overflow).

            namespace impl {

                ABEL_CONSTEXPR_FUNCTION bool is_leap_year(year_t y) noexcept {
                    return y % 4 == 0 && (y % 100 != 0 || y % 400 == 0);
                }

                ABEL_CONSTEXPR_FUNCTION int year_index(year_t y, month_t m) noexcept {
                    return (static_cast<int>((y + (m > 2)) % 400) + 400) % 400;
                }

                ABEL_CONSTEXPR_FUNCTION int days_per_century(year_t y, month_t m) noexcept {
                    const int yi = year_index(y, m);
                    return 36524 + (yi == 0 || yi > 300);
                }

                ABEL_CONSTEXPR_FUNCTION int days_per_4years(year_t y, month_t m) noexcept {
                    const int yi = year_index(y, m);
                    return 1460 + (yi == 0 || yi > 300 || (yi - 1) % 100 < 96);
                }

                ABEL_CONSTEXPR_FUNCTION int days_per_year(year_t y, month_t m) noexcept {
                    return is_leap_year(y + (m > 2)) ? 366 : 365;
                }

                ABEL_CONSTEXPR_FUNCTION int days_per_month(year_t y, month_t m) noexcept {
                    ABEL_CONSTEXPR_VARIABLE int k_days_per_month[1 + 12] = {
                            -1, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31  // non leap year
                    };
                    return k_days_per_month[m] + (m == 2 && is_leap_year(y));
                }

                ABEL_CONSTEXPR_FUNCTION fields n_day(year_t y, month_t m, diff_t d, diff_t cd, hour_t hh,
                                                     minute_t mm, second_t ss) noexcept {
                    y += (cd / 146097) * 400;
                    cd %= 146097;
                    if (cd < 0) {
                        y -= 400;
                        cd += 146097;
                    }
                    y += (d / 146097) * 400;
                    d = d % 146097 + cd;
                    if (d > 0) {
                        if (d > 146097) {
                            y += 400;
                            d -= 146097;
                        }
                    } else {
                        if (d > -365) {
// We often hit the previous year when stepping a civil time backwards,
// so special case it to avoid counting up by 100/4/1-year chunks.
                            y -= 1;
                            d += days_per_year(y, m);
                        } else {
                            y -= 400;
                            d += 146097;
                        }
                    }
                    if (d > 365) {
                        for (int n = days_per_century(y, m); d > n; n = days_per_century(y, m)) {
                            d -= n;
                            y += 100;
                        }
                        for (int n = days_per_4years(y, m); d > n; n = days_per_4years(y, m)) {
                            d -= n;
                            y += 4;
                        }
                        for (int n = days_per_year(y, m); d > n; n = days_per_year(y, m)) {
                            d -= n;
                            ++y;
                        }
                    }
                    if (d > 28) {
                        for (int n = days_per_month(y, m); d > n; n = days_per_month(y, m)) {
                            d -= n;
                            if (++m > 12) {
                                ++y;
                                m = 1;
                            }
                        }
                    }
                    return fields(y, m, static_cast<day_t>(d), hh, mm, ss);
                }

                ABEL_CONSTEXPR_FUNCTION fields n_mon(year_t y, diff_t m, diff_t d, diff_t cd, hour_t hh,
                                                     minute_t mm, second_t ss) noexcept {
                    if (m != 12) {
                        y += m / 12;
                        m %= 12;
                        if (m <= 0) {
                            y -= 1;
                            m += 12;
                        }
                    }
                    return n_day(y, static_cast<month_t>(m), d, cd, hh, mm, ss);
                }

                ABEL_CONSTEXPR_FUNCTION fields n_hour(year_t y, diff_t m, diff_t d, diff_t cd, diff_t hh,
                                                      minute_t mm, second_t ss) noexcept {
                    cd += hh / 24;
                    hh %= 24;
                    if (hh < 0) {
                        cd -= 1;
                        hh += 24;
                    }
                    return n_mon(y, m, d, cd, static_cast<hour_t>(hh), mm, ss);
                }

                ABEL_CONSTEXPR_FUNCTION fields n_min(year_t y, diff_t m, diff_t d, diff_t hh, diff_t ch,
                                                     diff_t mm, second_t ss) noexcept {
                    ch += mm / 60;
                    mm %= 60;
                    if (mm < 0) {
                        ch -= 1;
                        mm += 60;
                    }
                    return n_hour(y, m, d, hh / 24 + ch / 24, hh % 24 + ch % 24,
                                  static_cast<minute_t>(mm), ss);
                }

                ABEL_CONSTEXPR_FUNCTION fields n_sec(year_t y, diff_t m, diff_t d, diff_t hh, diff_t mm,
                                                     diff_t ss) noexcept {
// Optimization for when (non-constexpr) fields are already normalized.
                    if (0 <= ss && ss < 60) {
                        const second_t nss = static_cast<second_t>(ss);
                        if (0 <= mm && mm < 60) {
                            const minute_t nmm = static_cast<minute_t>(mm);
                            if (0 <= hh && hh < 24) {
                                const hour_t nhh = static_cast<hour_t>(hh);
                                if (1 <= d && d <= 28 && 1 <= m && m <= 12) {
                                    const day_t nd = static_cast<day_t>(d);
                                    const month_t nm = static_cast<month_t>(m);
                                    return fields(y, nm, nd, nhh, nmm, nss);
                                }
                                return n_mon(y, m, d, 0, nhh, nmm, nss);
                            }
                            return n_hour(y, m, d, hh / 24, hh % 24, nmm, nss);
                        }
                        return n_min(y, m, d, hh, mm / 60, mm % 60, nss);
                    }
                    diff_t cm = ss / 60;
                    ss %= 60;
                    if (ss < 0) {
                        cm -= 1;
                        ss += 60;
                    }
                    return n_min(y, m, d, hh, mm / 60 + cm / 60, mm % 60 + cm % 60,
                                 static_cast<second_t>(ss));
                }

            }  // namespace impl

////////////////////////////////////////////////////////////////////////

// Increments the indicated (normalized) field by "n".
            ABEL_CONSTEXPR_FUNCTION fields step(second_tag, fields f, diff_t n) noexcept {
                return impl::n_sec(f.y, f.m, f.d, f.hh, f.mm + n / 60, f.ss + n % 60);
            }

            ABEL_CONSTEXPR_FUNCTION fields step(minute_tag, fields f, diff_t n) noexcept {
                return impl::n_min(f.y, f.m, f.d, f.hh + n / 60, 0, f.mm + n % 60, f.ss);
            }

            ABEL_CONSTEXPR_FUNCTION fields step(hour_tag, fields f, diff_t n) noexcept {
                return impl::n_hour(f.y, f.m, f.d + n / 24, 0, f.hh + n % 24, f.mm, f.ss);
            }

            ABEL_CONSTEXPR_FUNCTION fields step(day_tag, fields f, diff_t n) noexcept {
                return impl::n_day(f.y, f.m, f.d, n, f.hh, f.mm, f.ss);
            }

            ABEL_CONSTEXPR_FUNCTION fields step(month_tag, fields f, diff_t n) noexcept {
                return impl::n_mon(f.y + n / 12, f.m + n % 12, f.d, 0, f.hh, f.mm, f.ss);
            }

            ABEL_CONSTEXPR_FUNCTION fields step(year_tag, fields f, diff_t n) noexcept {
                return fields(f.y + n, f.m, f.d, f.hh, f.mm, f.ss);
            }

////////////////////////////////////////////////////////////////////////

            namespace impl {

// Returns (v * f + a) but avoiding intermediate overflow when possible.
                ABEL_CONSTEXPR_FUNCTION diff_t scale_add(diff_t v, diff_t f, diff_t a) noexcept {
                    return (v < 0) ? ((v + 1) * f + a) - f : ((v - 1) * f + a) + f;
                }

// Map a (normalized) Y/M/D to the number of days before/after 1970-01-01.
// Probably overflows for years outside [-292277022656:292277026595].
                ABEL_CONSTEXPR_FUNCTION diff_t ymd_ord(year_t y, month_t m, day_t d) noexcept {
                    const diff_t eyear = (m <= 2) ? y - 1 : y;
                    const diff_t era = (eyear >= 0 ? eyear : eyear - 399) / 400;
                    const diff_t yoe = eyear - era * 400;
                    const diff_t doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
                    const diff_t doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
                    return era * 146097 + doe - 719468;
                }

// Returns the difference in days between two normalized Y-M-D tuples.
// ymd_ord() will encounter integer overflow given extreme year values,
// yet the difference between two such extreme values may actually be
// small, so we take a little care to avoid overflow when possible by
// exploiting the 146097-day cycle.
                ABEL_CONSTEXPR_FUNCTION diff_t day_difference(year_t y1, month_t m1, day_t d1, year_t y2,
                                                              month_t m2, day_t d2) noexcept {
                    const diff_t a_c4_off = y1 % 400;
                    const diff_t b_c4_off = y2 % 400;
                    diff_t c4_diff = (y1 - a_c4_off) - (y2 - b_c4_off);
                    diff_t delta = ymd_ord(a_c4_off, m1, d1) - ymd_ord(b_c4_off, m2, d2);
                    if (c4_diff > 0 && delta < 0) {
                        delta += 2 * 146097;
                        c4_diff -= 2 * 400;
                    } else if (c4_diff < 0 && delta > 0) {
                        delta -= 2 * 146097;
                        c4_diff += 2 * 400;
                    }
                    return (c4_diff / 400 * 146097) + delta;
                }

            }  // namespace impl

// Returns the difference between fields structs using the indicated unit.
            ABEL_CONSTEXPR_FUNCTION diff_t difference(year_tag, fields f1, fields f2) noexcept {
                return f1.y - f2.y;
            }

            ABEL_CONSTEXPR_FUNCTION diff_t difference(month_tag, fields f1, fields f2) noexcept {
                return impl::scale_add(difference(year_tag{}, f1, f2), 12, (f1.m - f2.m));
            }

            ABEL_CONSTEXPR_FUNCTION diff_t difference(day_tag, fields f1, fields f2) noexcept {
                return impl::day_difference(f1.y, f1.m, f1.d, f2.y, f2.m, f2.d);
            }

            ABEL_CONSTEXPR_FUNCTION diff_t difference(hour_tag, fields f1, fields f2) noexcept {
                return impl::scale_add(difference(day_tag{}, f1, f2), 24, (f1.hh - f2.hh));
            }

            ABEL_CONSTEXPR_FUNCTION diff_t difference(minute_tag, fields f1, fields f2) noexcept {
                return impl::scale_add(difference(hour_tag{}, f1, f2), 60, (f1.mm - f2.mm));
            }

            ABEL_CONSTEXPR_FUNCTION diff_t difference(second_tag, fields f1, fields f2) noexcept {
                return impl::scale_add(difference(minute_tag{}, f1, f2), 60, f1.ss - f2.ss);
            }

////////////////////////////////////////////////////////////////////////

// Aligns the (normalized) fields struct to the indicated field.
            ABEL_CONSTEXPR_FUNCTION fields align(second_tag, fields f) noexcept { return f; }

            ABEL_CONSTEXPR_FUNCTION fields align(minute_tag, fields f) noexcept {
                return fields{f.y, f.m, f.d, f.hh, f.mm, 0};
            }

            ABEL_CONSTEXPR_FUNCTION fields align(hour_tag, fields f) noexcept {
                return fields{f.y, f.m, f.d, f.hh, 0, 0};
            }

            ABEL_CONSTEXPR_FUNCTION fields align(day_tag, fields f) noexcept {
                return fields{f.y, f.m, f.d, 0, 0, 0};
            }

            ABEL_CONSTEXPR_FUNCTION fields align(month_tag, fields f) noexcept {
                return fields{f.y, f.m, 1, 0, 0, 0};
            }

            ABEL_CONSTEXPR_FUNCTION fields align(year_tag, fields f) noexcept {
                return fields{f.y, 1, 1, 0, 0, 0};
            }

////////////////////////////////////////////////////////////////////////

            namespace impl {

                template<typename H>
                H AbelHashValueImpl(second_tag, H h, fields f) {
                    return H::combine(std::move(h), f.y, f.m, f.d, f.hh, f.mm, f.ss);
                }

                template<typename H>
                H AbelHashValueImpl(minute_tag, H h, fields f) {
                    return H::combine(std::move(h), f.y, f.m, f.d, f.hh, f.mm);
                }

                template<typename H>
                H AbelHashValueImpl(hour_tag, H h, fields f) {
                    return H::combine(std::move(h), f.y, f.m, f.d, f.hh);
                }

                template<typename H>
                H AbelHashValueImpl(day_tag, H h, fields f) {
                    return H::combine(std::move(h), f.y, f.m, f.d);
                }

                template<typename H>
                H AbelHashValueImpl(month_tag, H h, fields f) {
                    return H::combine(std::move(h), f.y, f.m);
                }

                template<typename H>
                H AbelHashValueImpl(year_tag, H h, fields f) {
                    return H::combine(std::move(h), f.y);
                }

            }  // namespace impl

////////////////////////////////////////////////////////////////////////

            template<typename T>
            class civil_time {
            public:
                explicit ABEL_CONSTEXPR_MEMBER civil_time(year_t y, diff_t m = 1, diff_t d = 1,
                                                          diff_t hh = 0, diff_t mm = 0,
                                                          diff_t ss = 0) noexcept
                        : civil_time(impl::n_sec(y, m, d, hh, mm, ss)) {}

                ABEL_CONSTEXPR_MEMBER civil_time() noexcept : f_{1970, 1, 1, 0, 0, 0} {}

                civil_time(const civil_time &) = default;

                civil_time &operator=(const civil_time &) = default;

                // Conversion between civil times of different alignment. Conversion to
                // a more precise alignment is allowed implicitly (e.g., day -> hour),
                // but conversion where information is discarded must be explicit
                // (e.g., second -> minute).
                template<typename U, typename S>
                using preserves_data =
                typename std::enable_if<std::is_base_of<U, S>::value>::type;

                template<typename U>
                ABEL_CONSTEXPR_MEMBER civil_time(const civil_time<U> &ct,
                                                 preserves_data<T, U> * = nullptr) noexcept
                        : civil_time(ct.f_) {}

                template<typename U>
                explicit ABEL_CONSTEXPR_MEMBER civil_time(const civil_time<U> &ct,
                                                          preserves_data<U, T> * = nullptr) noexcept
                        : civil_time(ct.f_) {}

                // Factories for the maximum/minimum representable civil_time.
                static ABEL_CONSTEXPR_FUNCTION civil_time(max)() {
                    const auto max_year = (std::numeric_limits<std::int_least64_t>::max)();
                    return civil_time(max_year, 12, 31, 23, 59, 59);
                }

                static ABEL_CONSTEXPR_FUNCTION civil_time(min)() {
                    const auto min_year = (std::numeric_limits<std::int_least64_t>::min)();
                    return civil_time(min_year, 1, 1, 0, 0, 0);
                }

                // Field accessors.  Note: All but year() return an int.
                ABEL_CONSTEXPR_MEMBER year_t year() const noexcept { return f_.y; }

                ABEL_CONSTEXPR_MEMBER int month() const noexcept { return f_.m; }

                ABEL_CONSTEXPR_MEMBER int day() const noexcept { return f_.d; }

                ABEL_CONSTEXPR_MEMBER int hour() const noexcept { return f_.hh; }

                ABEL_CONSTEXPR_MEMBER int minute() const noexcept { return f_.mm; }

                ABEL_CONSTEXPR_MEMBER int second() const noexcept { return f_.ss; }

                // Assigning arithmetic.
                ABEL_CONSTEXPR_MEMBER civil_time &operator+=(diff_t n) noexcept {
                    f_ = step(T{}, f_, n);
                    return *this;
                }

                ABEL_CONSTEXPR_MEMBER civil_time &operator-=(diff_t n) noexcept {
                    if (n != (std::numeric_limits<diff_t>::min)()) {
                        f_ = step(T{}, f_, -n);
                    } else {
                        f_ = step(T{}, step(T{}, f_, -(n + 1)), 1);
                    }
                    return *this;
                }

                ABEL_CONSTEXPR_MEMBER civil_time &operator++() noexcept { return *this += 1; }

                ABEL_CONSTEXPR_MEMBER civil_time operator++(int) noexcept {
                    const civil_time a = *this;
                    ++*this;
                    return a;
                }

                ABEL_CONSTEXPR_MEMBER civil_time &operator--() noexcept { return *this -= 1; }

                ABEL_CONSTEXPR_MEMBER civil_time operator--(int) noexcept {
                    const civil_time a = *this;
                    --*this;
                    return a;
                }

                // Binary arithmetic operators.
                friend ABEL_CONSTEXPR_FUNCTION civil_time operator+(civil_time a, diff_t n) noexcept {
                    return a += n;
                }

                friend ABEL_CONSTEXPR_FUNCTION civil_time operator+(diff_t n, civil_time a) noexcept {
                    return a += n;
                }

                friend ABEL_CONSTEXPR_FUNCTION civil_time operator-(civil_time a, diff_t n) noexcept {
                    return a -= n;
                }

                friend ABEL_CONSTEXPR_FUNCTION diff_t operator-(civil_time lhs, civil_time rhs) noexcept {
                    return difference(T{}, lhs.f_, rhs.f_);
                }

                template<typename H>
                friend H AbelHashValue(H h, civil_time a) {
                    return impl::AbelHashValueImpl(T{}, std::move(h), a.f_);
                }

            private:
                // All instantiations of this template are allowed to call the following
                // private constructor and access the private fields member.
                template<typename U>
                friend
                class civil_time;

                // The designated constructor that all others eventually call.
                explicit ABEL_CONSTEXPR_MEMBER civil_time(fields f) noexcept : f_(align(T{}, f)) {}

                fields f_;
            };

// Disallows difference between differently aligned types.
// auto n = civil_day(...) - civil_hour(...);  // would be confusing.
            template<typename T, typename U>
            ABEL_CONSTEXPR_FUNCTION diff_t operator-(civil_time<T>, civil_time<U>) = delete;

            using civil_year = civil_time<year_tag>;
            using civil_month = civil_time<month_tag>;
            using civil_day = civil_time<day_tag>;
            using civil_hour = civil_time<hour_tag>;
            using civil_minute = civil_time<minute_tag>;
            using civil_second = civil_time<second_tag>;

////////////////////////////////////////////////////////////////////////

// Relational operators that work with differently aligned objects.
// Always compares all six fields.
            template<typename T1, typename T2>
            ABEL_CONSTEXPR_FUNCTION bool operator<(const civil_time<T1> &lhs,
                                                   const civil_time<T2> &rhs) noexcept {
                return (
                        lhs.year() < rhs.year() ||
                        (lhs.year() == rhs.year() &&
                         (lhs.month() < rhs.month() ||
                          (lhs.month() == rhs.month() &&
                           (lhs.day() < rhs.day() || (lhs.day() == rhs.day() &&
                                                      (lhs.hour() < rhs.hour() ||
                                                       (lhs.hour() == rhs.hour() &&
                                                        (lhs.minute() < rhs.minute() ||
                                                         (lhs.minute() == rhs.minute() &&
                                                          (lhs.second() < rhs.second())))))))))));
            }

            template<typename T1, typename T2>
            ABEL_CONSTEXPR_FUNCTION bool operator<=(const civil_time<T1> &lhs,
                                                    const civil_time<T2> &rhs) noexcept {
                return !(rhs < lhs);
            }

            template<typename T1, typename T2>
            ABEL_CONSTEXPR_FUNCTION bool operator>=(const civil_time<T1> &lhs,
                                                    const civil_time<T2> &rhs) noexcept {
                return !(lhs < rhs);
            }

            template<typename T1, typename T2>
            ABEL_CONSTEXPR_FUNCTION bool operator>(const civil_time<T1> &lhs,
                                                   const civil_time<T2> &rhs) noexcept {
                return rhs < lhs;
            }

            template<typename T1, typename T2>
            ABEL_CONSTEXPR_FUNCTION bool operator==(const civil_time<T1> &lhs,
                                                    const civil_time<T2> &rhs) noexcept {
                return lhs.year() == rhs.year() && lhs.month() == rhs.month() &&
                       lhs.day() == rhs.day() && lhs.hour() == rhs.hour() &&
                       lhs.minute() == rhs.minute() && lhs.second() == rhs.second();
            }

            template<typename T1, typename T2>
            ABEL_CONSTEXPR_FUNCTION bool operator!=(const civil_time<T1> &lhs,
                                                    const civil_time<T2> &rhs) noexcept {
                return !(lhs == rhs);
            }

////////////////////////////////////////////////////////////////////////

            enum class weekday {
                monday,
                tuesday,
                wednesday,
                thursday,
                friday,
                saturday,
                sunday,
            };

            ABEL_CONSTEXPR_FUNCTION weekday get_weekday(const civil_second &cs) noexcept {
                ABEL_CONSTEXPR_VARIABLE
                weekday k_weekday_by_mon_off[13] = {
                        weekday::monday, weekday::tuesday, weekday::wednesday,
                        weekday::thursday, weekday::friday, weekday::saturday,
                        weekday::sunday, weekday::monday, weekday::tuesday,
                        weekday::wednesday, weekday::thursday, weekday::friday,
                        weekday::saturday,
                };
                ABEL_CONSTEXPR_VARIABLE int k_weekday_offsets[1 + 12] = {
                        -1, 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4,
                };
                year_t wd = 2400 + (cs.year() % 400) - (cs.month() < 3);
                wd += wd / 4 - wd / 100 + wd / 400;
                wd += k_weekday_offsets[cs.month()] + cs.day();
                return k_weekday_by_mon_off[wd % 7 + 6];
            }

////////////////////////////////////////////////////////////////////////

            ABEL_CONSTEXPR_FUNCTION civil_day next_weekday(civil_day cd, weekday wd) noexcept {
                ABEL_CONSTEXPR_VARIABLE
                weekday k_weekdays_forw[14] = {
                        weekday::monday, weekday::tuesday, weekday::wednesday,
                        weekday::thursday, weekday::friday, weekday::saturday,
                        weekday::sunday, weekday::monday, weekday::tuesday,
                        weekday::wednesday, weekday::thursday, weekday::friday,
                        weekday::saturday, weekday::sunday,
                };
                weekday base = get_weekday(cd);
                for (int i = 0;; ++i) {
                    if (base == k_weekdays_forw[i]) {
                        for (int j = i + 1;; ++j) {
                            if (wd == k_weekdays_forw[j]) {
                                return cd + (j - i);
                            }
                        }
                    }
                }
            }

            ABEL_CONSTEXPR_FUNCTION civil_day prev_weekday(civil_day cd, weekday wd) noexcept {
                ABEL_CONSTEXPR_VARIABLE
                weekday k_weekdays_back[14] = {
                        weekday::sunday, weekday::saturday, weekday::friday,
                        weekday::thursday, weekday::wednesday, weekday::tuesday,
                        weekday::monday, weekday::sunday, weekday::saturday,
                        weekday::friday, weekday::thursday, weekday::wednesday,
                        weekday::tuesday, weekday::monday,
                };
                weekday base = get_weekday(cd);
                for (int i = 0;; ++i) {
                    if (base == k_weekdays_back[i]) {
                        for (int j = i + 1;; ++j) {
                            if (wd == k_weekdays_back[j]) {
                                return cd - (j - i);
                            }
                        }
                    }
                }
            }

            ABEL_CONSTEXPR_FUNCTION int get_yearday(const civil_second &cs) noexcept {
                ABEL_CONSTEXPR_VARIABLE int k_month_offsets[1 + 12] = {
                        -1, 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334,
                };
                const int feb29 = (cs.month() > 2 && impl::is_leap_year(cs.year()));
                return k_month_offsets[cs.month()] + feb29 + cs.day();
            }

////////////////////////////////////////////////////////////////////////

            std::ostream &operator<<(std::ostream &os, const civil_year &y);

            std::ostream &operator<<(std::ostream &os, const civil_month &m);

            std::ostream &operator<<(std::ostream &os, const civil_day &d);

            std::ostream &operator<<(std::ostream &os, const civil_hour &h);

            std::ostream &operator<<(std::ostream &os, const civil_minute &m);

            std::ostream &operator<<(std::ostream &os, const civil_second &s);

            std::ostream &operator<<(std::ostream &os, weekday wd);

        }  // namespace detail

    } //namespace chrono_internal
} //namespace abel
#endif //ABEL_CHRONO_INTERNAL_CHRONO_TIME_DETAIL_H_
