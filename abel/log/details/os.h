// Copyright(c) 2015-present, Gabi Melman & spdlog contributors.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#pragma once

#include <ctime> // std::time_t
#include "abel/log/common.h"

namespace abel {
namespace details {
namespace os {

ABEL_API abel::log_clock::time_point now() ABEL_NOEXCEPT;

ABEL_API std::tm localtime(const std::time_t &time_tt) ABEL_NOEXCEPT;

ABEL_API std::tm localtime() ABEL_NOEXCEPT;

ABEL_API std::tm gmtime(const std::time_t &time_tt) ABEL_NOEXCEPT;

ABEL_API std::tm gmtime() ABEL_NOEXCEPT;

// eol definition
#if !defined(LOG_EOL)
#ifdef _WIN32
#define LOG_EOL "\r\n"
#else
#define LOG_EOL "\n"
#endif
#endif

ABEL_CONSTEXPR static const char *default_eol = LOG_EOL;

// folder separator
#ifdef _WIN32
static const char folder_sep = '\\';
#else
ABEL_CONSTEXPR static const char folder_sep = '/';
#endif

// fopen_s on non windows for writing
ABEL_API bool fopen_s(FILE **fp, const filename_t &filename, const filename_t &mode);

// Remove filename. return 0 on success
ABEL_API int remove(const filename_t &filename) ABEL_NOEXCEPT;

// Remove file if exists. return 0 on success
// Note: Non atomic (might return failure to delete if concurrently deleted by other process/thread)
ABEL_API int remove_if_exists(const filename_t &filename) ABEL_NOEXCEPT;

ABEL_API int rename(const filename_t &filename1, const filename_t &filename2) ABEL_NOEXCEPT;

// Return if file exists.
ABEL_API bool path_exists(const filename_t &filename) ABEL_NOEXCEPT;

// Return file size according to open FILE* object
ABEL_API size_t filesize(FILE *f);

// Return utc offset in minutes or throw log_ex on failure
ABEL_API int utc_minutes_offset(const std::tm &tm = details::os::localtime());

// Return current thread id as size_t
// It exists because the std::this_thread::get_id() is much slower(especially
// under VS 2013)
ABEL_API size_t _thread_id() ABEL_NOEXCEPT;

// Return current thread id as size_t (from thread local storage)
ABEL_API size_t thread_id() ABEL_NOEXCEPT;

// This is avoid msvc issue in sleep_for that happens if the clock changes.
// See https://github.com/gabime/spdlog/issues/609
ABEL_API void sleep_for_millis(int milliseconds) ABEL_NOEXCEPT;

ABEL_API std::string filename_to_str(const filename_t &filename);

ABEL_API int pid() ABEL_NOEXCEPT;

// Determine if the terminal supports colors
// Source: https://github.com/agauniyal/rang/
ABEL_API bool is_color_terminal() ABEL_NOEXCEPT;

// Determine if the terminal attached
// Source: https://github.com/agauniyal/rang/
ABEL_API bool in_terminal(FILE *file) ABEL_NOEXCEPT;

#if (defined(LOG_WCHAR_TO_UTF8_SUPPORT) || defined(LOG_WCHAR_FILENAMES)) && defined(_WIN32)
ABEL_API void wstr_to_utf8buf(wstring_view_t wstr, memory_buf_t &target);
#endif

// Return directory name from given path or empty string
// "abc/file" => "abc"
// "abc/" => "abc"
// "abc" => ""
// "abc///" => "abc//"
ABEL_API filename_t dir_name(filename_t path);

// Create a dir from the given path.
// Return true if succeeded or if this dir already exists.
ABEL_API bool create_dir(filename_t path);

// non thread safe, cross platform getenv/getenv_s
// return empty string if field not found
ABEL_API std::string getenv(const char *field);

} // namespace os
} // namespace details
}  // namespace abel

#include "abel/log/details/os_inl.h"
