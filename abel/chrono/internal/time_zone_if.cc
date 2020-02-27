//
// Created by liyinbin on 2020/1/28.
//


#include <abel/chrono/internal/time_zone_if.h>
#include <abel/base/profile.h>
#include <abel/chrono/internal/time_zone_info.h>
#include <abel/chrono/internal/time_zone_libc.h>

namespace abel {

    namespace chrono_internal {

        std::unique_ptr<time_zone_if> time_zone_if::load(const std::string &name) {
            // Support "libc:localtime" and "libc:*" to access the legacy
            // localtime and UTC support respectively from the C library.
            if (name.compare(0, 5, "libc:") == 0) {
                return std::unique_ptr<time_zone_if>(new time_zone_libc(name.substr(5)));
            }

            // Otherwise use the "zoneinfo" implementation by default.
            std::unique_ptr<time_zone_info> tz(new time_zone_info);
            if (!tz->load(name)) tz.reset();
            return std::unique_ptr<time_zone_if>(tz.release());
        }

// Defined out-of-line to avoid emitting a weak vtable in all TUs.
        time_zone_if::~time_zone_if() {}

    }  // namespace chrono_internal

}  // namespace abel
