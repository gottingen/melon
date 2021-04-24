// Copyright(c) 2015-present, Gabi Melman & spdlog contributors.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#pragma once


#include "abel/log/details/os.h"

namespace abel {
namespace details {

ABEL_FORCE_INLINE log_msg::log_msg(abel::log_clock::time_point log_time, abel::source_loc loc,
                                   std::string_view a_logger_name,
                                   abel::level::level_enum lvl, std::string_view msg)
        : logger_name(a_logger_name), level(lvl), time(log_time)
#ifndef LOG_NO_THREAD_ID
        , thread_id(os::thread_id())
#endif
        , source(loc), payload(msg) {}

ABEL_FORCE_INLINE log_msg::log_msg(
        abel::source_loc loc, std::string_view a_logger_name, abel::level::level_enum lvl, std::string_view msg)
        : log_msg(os::now(), loc, a_logger_name, lvl, msg) {}

ABEL_FORCE_INLINE log_msg::log_msg(std::string_view a_logger_name, abel::level::level_enum lvl,
                                   std::string_view msg)
        : log_msg(os::now(), source_loc{}, a_logger_name, lvl, msg) {}

} // namespace details
}  // namespace abel
