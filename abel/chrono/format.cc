// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com

#include <string.h>
#include <cctype>
#include <cstdint>

#include "abel/chrono/internal/time_zone.h"
#include "abel/chrono/time.h"

namespace abel {

extern const char RFC3339_full[] = "%Y-%m-%dT%H:%M:%E*S%Ez";
extern const char RFC3339_sec[] = "%Y-%m-%dT%H:%M:%S%Ez";

extern const char RFC1123_full[] = "%a, %d %b %E4Y %H:%M:%S %z";
extern const char RFC1123_no_wday[] = "%d %b %E4Y %H:%M:%S %z";

namespace {

const char kInfiniteFutureStr[] = "infinite-future";
const char kInfinitePastStr[] = "infinite-past";

struct cctz_parts {
    abel::chrono_internal::time_point<abel::chrono_internal::seconds> sec;
    abel::chrono_internal::detail::femtoseconds fem;
};

ABEL_FORCE_INLINE abel::chrono_internal::time_point<abel::chrono_internal::seconds> unix_epoch() {
    return std::chrono::time_point_cast<abel::chrono_internal::seconds>(
            std::chrono::system_clock::from_time_t(0));
}

// Splits a time_point into seconds and femtoseconds, which can be used with CCTZ.
// Requires that 't' is finite. See duration.cc for details about rep_hi and
// rep_lo.
cctz_parts Split(abel::time_point t) {
    const auto d = time_point::to_unix_duration(t);
    const int64_t rep_hi = duration::get_rep_hi(d);
    const int64_t rep_lo = duration::get_rep_lo(d);
    const auto sec = unix_epoch() + abel::chrono_internal::seconds(rep_hi);
    const auto fem = abel::chrono_internal::detail::femtoseconds(rep_lo * (1000 * 1000 / 4));
    return {sec, fem};
}

// Joins the given seconds and femtoseconds into a time_point. See duration.cc for
// details about rep_hi and rep_lo.
abel::time_point Join(const cctz_parts &parts) {
    const int64_t rep_hi = (parts.sec - unix_epoch()).count();
    const uint32_t rep_lo = parts.fem.count() / (1000 * 1000 / 4);
    const auto d = duration::make_duration(rep_hi, rep_lo);
    return time_point::from_unix_duration(d);
}

}  // namespace

std::string format_time(const std::string &format, abel::time_point t,
                        abel::time_zone tz) {
    if (t == abel::time_point::infinite_future())
        return kInfiniteFutureStr;
    if (t == abel::time_point::infinite_past())
        return kInfinitePastStr;
    const auto parts = Split(t);
    return abel::chrono_internal::detail::format(format, parts.sec, parts.fem,
                                                 abel::chrono_internal::time_zone(tz));
}

std::string format_time(abel::time_point t, abel::time_zone tz) {
    return format_time(RFC3339_full, t, tz);
}

std::string format_time(abel::time_point t) {
    return abel::format_time(RFC3339_full, t, abel::local_time_zone());
}

bool parse_time(const std::string &format, const std::string &input,
                abel::time_point *time, std::string *err) {
    return abel::parse_time(format, input, abel::utc_time_zone(), time, err);
}

// If the input string does not contain an explicit UTC offset, interpret
// the fields with respect to the given time_zone.
bool parse_time(const std::string &format, const std::string &input,
                abel::time_zone tz, abel::time_point *time, std::string *err) {
    const char *data = input.c_str();
    while (std::isspace(*data))
        ++data;

    size_t inf_size = strlen(kInfiniteFutureStr);
    if (strncmp(data, kInfiniteFutureStr, inf_size) == 0) {
        const char *new_data = data + inf_size;
        while (std::isspace(*new_data))
            ++new_data;
        if (*new_data == '\0') {
            *time = time_point::infinite_future();
            return true;
        }
    }

    inf_size = strlen(kInfinitePastStr);
    if (strncmp(data, kInfinitePastStr, inf_size) == 0) {
        const char *new_data = data + inf_size;
        while (std::isspace(*new_data))
            ++new_data;
        if (*new_data == '\0') {
            *time = time_point::infinite_past();
            return true;
        }
    }

    std::string error;
    cctz_parts parts;
    const bool b = abel::chrono_internal::detail::parse(format, input, abel::chrono_internal::time_zone(tz),
                                                        &parts.sec, &parts.fem, &error);
    if (b) {
        *time = Join(parts);
    } else if (err != nullptr) {
        *err = error;
    }
    return b;
}

// Functions required to support abel::time_point flags.
bool abel_parse_flag(std::string_view text, abel::time_point *t, std::string *error) {
    return abel::parse_time(RFC3339_full, std::string(text), abel::utc_time_zone(),
                            t, error);
}

std::string abel_unparse_flag(abel::time_point t) {
    return abel::format_time(RFC3339_full, t, abel::utc_time_zone());
}

/*
bool parse_flag(const std::string &text, abel::time_point *t, std::string *error) {
    return abel::parse_time(RFC3339_full, text, abel::utc_time_zone(), t, error);
}

std::string unparse_flag(abel::time_point t) {
    return abel::format_time(RFC3339_full, t, abel::utc_time_zone());
}
 */

}  // namespace abel
