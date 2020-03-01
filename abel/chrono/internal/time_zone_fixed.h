//
// Created by liyinbin on 2020/1/28.
//

#ifndef ABEL_CHRONO_INTERNAL_TIME_ZONE_FIXED_H_
#define ABEL_CHRONO_INTERNAL_TIME_ZONE_FIXED_H_

#include <string>
#include <abel/base/profile.h>
#include <abel/chrono/internal/time_zone.h>

namespace abel {
    namespace chrono_internal {

// Helper functions for dealing with the names and abbreviations
// of time zones that are a fixed offset (seconds east) from UTC.
// FixedOffsetFromName() extracts the offset from a valid fixed-offset
// name, while FixedOffsetToName() and FixedOffsetToAbbr() generate
// the canonical zone name and abbreviation respectively for the given
// offset.
//
// A fixed-offset name looks like "Fixed/UTC<+-><hours>:<mins>:<secs>".
// Its abbreviation is of the form "UTC(<+->H?H(MM(SS)?)?)?" where the
// optional pieces are omitted when their values are zero.  (Note that
// the sign is the opposite of that used in a POSIX TZ specification.)
//
// Note: FixedOffsetFromName() fails on syntax errors or when the parsed
// offset exceeds 24 hours.  FixedOffsetToName() and FixedOffsetToAbbr()
// both produce "UTC" when the argument offset exceeds 24 hours.
        bool fixed_offset_from_name(const std::string &name, seconds *offset);

        std::string fixed_offset_to_name(const seconds &offset);

        std::string fixed_offset_to_abbr(const seconds &offset);

    } //namespace chrono_internal
} //namespace abel

#endif //ABEL_CHRONO_INTERNAL_TIME_ZONE_FIXED_H_
