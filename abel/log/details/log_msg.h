// Copyright(c) 2015-present, Gabi Melman & spdlog contributors.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#pragma once

#include <string>
#include "abel/log/common.h"

namespace abel {
namespace details {
struct ABEL_API log_msg {
    log_msg() = default;

    log_msg(log_clock::time_point log_time, source_loc loc, std::string_view logger_name, level::level_enum lvl,
            std::string_view msg);

    log_msg(source_loc loc, std::string_view logger_name, level::level_enum lvl, std::string_view msg);

    log_msg(std::string_view logger_name, level::level_enum lvl, std::string_view msg);

    log_msg(const log_msg &other) = default;

    std::string_view logger_name;
    level::level_enum level{level::off};
    log_clock::time_point time;
    size_t thread_id{0};

    // wrapping the formatted text with color (updated by pattern_formatter).
    mutable size_t color_range_start{0};
    mutable size_t color_range_end{0};

    source_loc source;
    std::string_view payload;
};
} // namespace details
}  // namespace abel

#include "abel/log/details/log_msg_inl.h"
