
#ifndef ABEL_LOG_COMMON_H_
#define ABEL_LOG_COMMON_H_

#include <atomic>
#include <chrono>
#include <functional>
#include <initializer_list>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>

#if !defined(ABEL_WCHAR_T_NON_NATIVE) && defined(_WIN32)
#include <codecvt>
#include <locale>
#endif

#include <abel/base/profile.h>
#include <abel/log/details/null_mutex.h>
#include <abel/asl/format.h>

namespace abel {
    namespace log {
        class formatter;

        namespace sinks {
            class sink;
        }

        using sink_ptr = std::shared_ptr<sinks::sink>;
        using sinks_init_list = std::initializer_list<sink_ptr>;
        using log_err_handler = std::function<void(const std::string &err_msg)>;
        using level_t = std::atomic<int>;

        enum level_enum {
            trace = 0,
            debug = 1,
            info = 2,
            warn = 3,
            err = 4,
            critical = 5,
            off = 6
        };

#if !defined(ABEL_LOG_LEVEL_NAMES)
#define ABEL_LOG_LEVEL_NAMES                                                                                                                 \
    {                                                                                                                                      \
        "trace", "debug", "info", "warning", "error", "critical", "off"                                                                    \
    }
#endif
        static const char *level_names[]ABEL_LOG_LEVEL_NAMES;

        static const char *short_level_names[]{"T", "D", "I", "W", "E", "C", "O"};

        inline const char *to_c_str(abel::log::level_enum l) {
            return level_names[l];
        }

        inline const char *to_short_c_str(abel::log::level_enum l) {
            return short_level_names[l];
        }

        inline abel::log::level_enum from_str(const std::string &name) {
            static std::unordered_map<std::string, level_enum> name_to_level = // map string->level
                    {{level_names[0], trace},                               // trace
                     {level_names[1], debug},                            // debug
                     {level_names[2], info},                             // info
                     {level_names[3], warn},                             // warn
                     {level_names[4], err},                              // err
                     {level_names[5], critical},                         // critical
                     {level_names[6], off}};                             // off

            auto lvl_it = name_to_level.find(name);
            return lvl_it != name_to_level.end() ? lvl_it->second : off;
        }

        using level_hasher = std::hash<int>;

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
        class log_ex : public std::runtime_error {
        public:
            explicit log_ex(const std::string &msg)
                    : runtime_error(msg) {
            }

            log_ex(std::string msg, int last_errno)
                    : runtime_error(std::move(msg)), last_errno_(last_errno) {
            }

            const char *what() const ABEL_NOEXCEPT override {
                if (last_errno_) {
                    fmt::memory_buffer buf;
                    std::string msg(runtime_error::what());
                    fmt::format_system_error(buf, last_errno_, msg);
                    return fmt::to_string(buf).c_str();
                } else {
                    return runtime_error::what();
                }
            }

        private:
            int last_errno_{0};
        };

//
// wchar support for windows file names (ABEL_WCHAR_T_NON_NATIVE must not be defined)
//
#if defined(_WIN32) && !defined(ABEL_WCHAR_T_NON_NATIVE)
        using filename_t = std::wstring;
#else
        using filename_t = std::string;
#endif

#define ABEL_LOG_CATCH_AND_HANDLE                                                                                                            \
    catch (const std::exception &ex)                                                                                                       \
    {                                                                                                                                      \
        err_handler_(ex.what());                                                                                                           \
    }                                                                                                                                      \
    catch (...)                                                                                                                            \
    {                                                                                                                                      \
        err_handler_("Unknown exeption in logger");                                                                                        \
    }
    } //namespace log
} // namespace abel

#endif//ABEL_LOG_COMMON_H_