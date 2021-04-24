// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com

#ifndef ABEL_ABEL_CHRONO_INTERNAL_TIME_ZONE_LIBC_H_
#define ABEL_ABEL_CHRONO_INTERNAL_TIME_ZONE_LIBC_H_

#include <string>
#include "abel/base/profile.h"
#include "abel/chrono/internal/time_zone_if.h"

namespace abel {
namespace chrono_internal {


// A time zone backed by gmtime_r(3), localtime_r(3), and mktime(3),
// and which therefore only supports UTC and the local time zone.
// TODO: Add support for fixed offsets from UTC.
class time_zone_libc : public time_zone_if {
public:
    explicit time_zone_libc(const std::string &name);

    // TimeZoneIf implementations.
    time_zone::absolute_lookup break_time(
            const time_point<seconds> &tp) const override;

    time_zone::civil_lookup make_time(const civil_second &cs) const override;

    bool next_transition(const time_point<seconds> &tp,
                         time_zone::civil_transition *trans) const override;

    bool prev_transition(const time_point<seconds> &tp,
                         time_zone::civil_transition *trans) const override;

    std::string version() const override;

    std::string description() const override;

private:
    const bool local_;  // localtime or UTC
};

}  // namespace chrono_internal
}  // namespace abel

#endif  // ABEL_ABEL_CHRONO_INTERNAL_TIME_ZONE_LIBC_H_
