// Copyright(c) 2015-present, Gabi Melman & spdlog contributors.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#pragma once

#include <atomic>
#include <chrono>
#include <initializer_list>
#include <memory>
#include <exception>
#include <string>
#include <type_traits>
#include <functional>
#include "abel/log/tweakme.h"
#include "abel/log/details/null_mutex.h"
#include "abel/base/profile.h"
#include "abel/strings/format.h"

namespace abel {

class log_formatter;

namespace sinks {
class sink;
}

#if defined(_WIN32) && defined(LOG_WCHAR_FILENAMES)
using filename_t = std::wstring;
#define LOG_FILENAME_T(s) L##s
#else
using filename_t = std::string;
#define LOG_FILENAME_T(s) s
#endif

using log_clock = std::chrono::system_clock;
using sink_ptr = std::shared_ptr<sinks::sink>;
using sinks_init_list = std::initializer_list<sink_ptr>;
using err_handler = std::function<void(const std::string &err_msg)>;
using wstring_view_t = abel::basic_string_view<wchar_t>;
using memory_buf_t = abel::basic_memory_buffer<char, 250>;

#ifdef LOG_WCHAR_TO_UTF8_SUPPORT
#ifndef _WIN32
#error LOG_WCHAR_TO_UTF8_SUPPORT only supported on windows
#else
template<typename T>
struct is_convertible_to_wstring_view : std::is_convertible<T, wstring_view_t>
{};
#endif // _WIN32
#else
template<typename>
struct is_convertible_to_wstring_view : std::false_type {
};
#endif // LOG_WCHAR_TO_UTF8_SUPPORT

using level_t = std::atomic<int>;

#define LOG_LEVEL_TRACE 0
#define LOG_LEVEL_DEBUG 1
#define LOG_LEVEL_INFO 2
#define LOG_LEVEL_WARN 3
#define LOG_LEVEL_ERROR 4
#define LOG_LEVEL_CRITICAL 5
#define LOG_LEVEL_OFF 6

#if !defined(LOG_ACTIVE_LEVEL)
#define LOG_ACTIVE_LEVEL LOG_LEVEL_TRACE
#endif

// Log level enum
namespace level {
enum level_enum {
    trace = LOG_LEVEL_TRACE,
    debug = LOG_LEVEL_DEBUG,
    info = LOG_LEVEL_INFO,
    warn = LOG_LEVEL_WARN,
    err = LOG_LEVEL_ERROR,
    critical = LOG_LEVEL_CRITICAL,
    off = LOG_LEVEL_OFF,
    n_levels
};

#if !defined(LOG_LEVEL_NAMES)
#define LOG_LEVEL_NAMES                                                                                                                 \
    {                                                                                                                                      \
        "trace", "debug", "info", "warning", "error", "critical", "off"                                                                    \
    }
#endif

#if !defined(LOG_SHORT_LEVEL_NAMES)

#define LOG_SHORT_LEVEL_NAMES                                                                                                           \
    {                                                                                                                                      \
        "T", "D", "I", "W", "E", "C", "O"                                                                                                  \
    }
#endif

ABEL_API std::string_view &to_string_view(abel::level::level_enum l) ABEL_NOEXCEPT;

ABEL_API const char *to_short_c_str(abel::level::level_enum l) ABEL_NOEXCEPT;

ABEL_API abel::level::level_enum from_str(const std::string &name) ABEL_NOEXCEPT;

using level_hasher = std::hash<int>;
} // namespace level

//
// Color mode used by sinks with color support.
//
enum class color_mode {
    always,
    automatic,
    never
};

//
// Pattern time - specific time getting to use for pattern_formatter.
// local time by default
//
enum class pattern_time_type {
    local, // log localtime
    utc    // log utc
};

//
// Log exception
//
class ABEL_API log_ex : public std::exception {
  public:
    explicit log_ex(std::string msg);

    log_ex(const std::string &msg, int last_errno);

    const char *what() const ABEL_NOEXCEPT override;

  private:
    std::string msg_;
};

ABEL_API ABEL_NORETURN void throw_log_ex(const std::string &msg, int last_errno);

ABEL_API ABEL_NORETURN void throw_log_ex(std::string msg);

struct source_loc {
    ABEL_CONSTEXPR source_loc() = default;

    ABEL_CONSTEXPR source_loc(const char *filename_in, int line_in, const char *funcname_in)
            : filename{filename_in}, line{line_in}, funcname{funcname_in} {}

    ABEL_CONSTEXPR bool empty() const ABEL_NOEXCEPT {
        return line == 0;
    }

    const char *filename{nullptr};
    int line{0};
    const char *funcname{nullptr};
};

}  // namespace abel

#include "abel/log/common_inl.h"
