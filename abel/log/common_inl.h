// Copyright(c) 2015-present, Gabi Melman & spdlog contributors.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#pragma once


namespace abel {
namespace level {
static std::string_view level_string_views[]
LOG_LEVEL_NAMES;

static const char *short_level_names[]
LOG_SHORT_LEVEL_NAMES;

ABEL_FORCE_INLINE std::string_view
&
to_string_view(abel::level::level_enum
l) ABEL_NOEXCEPT {
return level_string_views[l];
}

ABEL_FORCE_INLINE const char *to_short_c_str(abel::level::level_enum
l) ABEL_NOEXCEPT {
return short_level_names[l];
}

ABEL_FORCE_INLINE abel::level::level_enum

from_str(const std::string &name)

ABEL_NOEXCEPT {
int level = 0;
for (
const auto &level_str
: level_string_views) {
if (level_str == name) {
return static_cast
<level::level_enum>(level);
}
level++;
}
// check also for "warn" and "err" before giving up..
if (name == "warn") {
return
level::warn;
}
if (name == "err") {
return
level::err;
}
return
level::off;
}
} // namespace level

ABEL_FORCE_INLINE log_ex::log_ex(std::string msg)
        : msg_(std::move(msg)) {}

ABEL_FORCE_INLINE log_ex::log_ex(const std::string &msg, int last_errno) {
    abel::memory_buf_t outbuf;
    abel::format_system_error(outbuf, last_errno, msg);
    msg_ = abel::to_string(outbuf);
}

ABEL_FORCE_INLINE const char *log_ex::what() const

ABEL_NOEXCEPT {
return msg_.

c_str();

}

ABEL_FORCE_INLINE void throw_log_ex(const std::string &msg, int last_errno) {
    ABEL_THROW(log_ex(msg, last_errno));
}

ABEL_FORCE_INLINE void throw_log_ex(std::string msg) {
    ABEL_THROW(log_ex(std::move(msg)));
}

}  // namespace abel

