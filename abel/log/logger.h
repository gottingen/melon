// Copyright(c) 2015-present, Gabi Melman & spdlog contributors.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#pragma once

// Thread safe logger (except for set_error_handler())
// Has name, log level, vector of std::shared sink pointers and log_formatter
// Upon each log write the logger:
// 1. Checks if its log level is enough to log the message and if yes:
// 2. Call the underlying sinks to do the job.
// 3. Each sink use its own private copy of a log_formatter to format the message
// and send to its destination.
//
// The use of private log_formatter per sink provides the opportunity to cache some
// formatted data, and support for different format per sink.

#include "abel/log/common.h"
#include "abel/log/details/log_msg.h"
#include "abel/log/details/backtracer.h"

#ifdef LOG_WCHAR_TO_UTF8_SUPPORT
#include "abel/log/details/os.h"
#endif

#include <vector>

#ifndef LOG_NO_EXCEPTIONS
#define LOG_LOGGER_CATCH()                                                                                                              \
    catch (const std::exception &ex)                                                                                                       \
    {                                                                                                                                      \
        err_handler_(ex.what());                                                                                                           \
    }                                                                                                                                      \
    catch (...)                                                                                                                            \
    {                                                                                                                                      \
        err_handler_("Unknown exception in logger");                                                                                       \
    }
#else
#define LOG_LOGGER_CATCH()
#endif

namespace abel {

    class ABEL_API logger {
    public:
        // Empty logger
        explicit logger(std::string name)
                : name_(std::move(name)), sinks_() {}

        // Logger with range on sinks
        template<typename It>
        logger(std::string name, It begin, It end)
                : name_(std::move(name)), sinks_(begin, end) {}

        // Logger with single sink
        logger(std::string name, sink_ptr single_sink)
                : logger(std::move(name), {std::move(single_sink)}) {}

        // Logger with sinks init list
        logger(std::string name, sinks_init_list sinks)
                : logger(std::move(name), sinks.begin(), sinks.end()) {}

        virtual ~logger() = default;

        logger(const logger &other);

        logger(logger &&other) ABEL_NOEXCEPT;

        logger &operator=(logger other) ABEL_NOEXCEPT;

        void swap(abel::logger &other) ABEL_NOEXCEPT;

        // FormatString is a type derived from abel::compile_string
        template<typename FormatString, typename std::enable_if<abel::is_compile_string<FormatString>::value, int>::type = 0, typename... Args>
        void log(source_loc loc, level::level_enum lvl, const FormatString &fmt, const Args &... args) {
            log_impl(loc, lvl, fmt, args...);
        }

        // FormatString is NOT a type derived from abel::compile_string but is a std::string_view or can be implicitly converted to one
        template<typename... Args>
        void log(source_loc loc, level::level_enum lvl, std::string_view fmt, const Args &... args) {
            log_impl(loc, lvl, fmt, args...);
        }

        template<typename FormatString, typename... Args>
        void log(level::level_enum lvl, const FormatString &fmt, const Args &... args) {
            log(source_loc{}, lvl, fmt, args...);
        }

        template<typename FormatString, typename... Args>
        void trace(const FormatString &fmt, const Args &... args) {
            log(level::trace, fmt, args...);
        }

        template<typename FormatString, typename... Args>
        void debug(const FormatString &fmt, const Args &... args) {
            log(level::debug, fmt, args...);
        }

        template<typename FormatString, typename... Args>
        void info(const FormatString &fmt, const Args &... args) {
            log(level::info, fmt, args...);
        }

        template<typename FormatString, typename... Args>
        void warn(const FormatString &fmt, const Args &... args) {
            log(level::warn, fmt, args...);
        }

        template<typename FormatString, typename... Args>
        void error(const FormatString &fmt, const Args &... args) {
            log(level::err, fmt, args...);
        }

        template<typename FormatString, typename... Args>
        void critical(const FormatString &fmt, const Args &... args) {
            log(level::critical, fmt, args...);
        }

        template<typename T>
        void log(level::level_enum lvl, const T &msg) {
            log(source_loc{}, lvl, msg);
        }

        // T can be statically converted to string_view and isn't a abel::compile_string
        template<class T, typename std::enable_if<
                std::is_convertible<const T &, std::string_view>::value &&
                !abel::is_compile_string<T>::value, int>::type = 0>
        void log(source_loc loc, level::level_enum lvl, const T &msg) {
            log(loc, lvl, std::string_view{msg});
        }

        void log(log_clock::time_point log_time, source_loc loc, level::level_enum lvl, std::string_view msg) {
            bool log_enabled = should_log(lvl);
            bool traceback_enabled = tracer_.enabled();
            if (!log_enabled && !traceback_enabled) {
                return;
            }

            details::log_msg log_msg(log_time, loc, name_, lvl, msg);
            log_it_impl(log_msg, log_enabled, traceback_enabled);
        }

        void log(source_loc loc, level::level_enum lvl, std::string_view msg) {
            bool log_enabled = should_log(lvl);
            bool traceback_enabled = tracer_.enabled();
            if (!log_enabled && !traceback_enabled) {
                return;
            }

            details::log_msg log_msg(loc, name_, lvl, msg);
            log_it_impl(log_msg, log_enabled, traceback_enabled);
        }

        void log(level::level_enum lvl, std::string_view msg) {
            log(source_loc{}, lvl, msg);
        }

        // T cannot be statically converted to string_view or wstring_view
        template<class T, typename std::enable_if<!std::is_convertible<const T &, std::string_view>::value &&
                                                  !is_convertible_to_wstring_view<const T &>::value,
                int>::type = 0>
        void log(source_loc loc, level::level_enum lvl, const T &msg) {
            log(loc, lvl, "{}", msg);
        }

        template<typename T>
        void trace(const T &msg) {
            log(level::trace, msg);
        }

        template<typename T>
        void debug(const T &msg) {
            log(level::debug, msg);
        }

        template<typename T>
        void info(const T &msg) {
            log(level::info, msg);
        }

        template<typename T>
        void warn(const T &msg) {
            log(level::warn, msg);
        }

        template<typename T>
        void error(const T &msg) {
            log(level::err, msg);
        }

        template<typename T>
        void critical(const T &msg) {
            log(level::critical, msg);
        }

#ifdef LOG_WCHAR_TO_UTF8_SUPPORT
#ifndef _WIN32
#error LOG_WCHAR_TO_UTF8_SUPPORT only supported on windows
#else

        template<typename... Args>
        void log(source_loc loc, level::level_enum lvl, wstring_view_t fmt, const Args &... args)
        {
            bool log_enabled = should_log(lvl);
            bool traceback_enabled = tracer_.enabled();
            if (!log_enabled && !traceback_enabled)
            {
                return;
            }
            ABEL_TRY
            {
                // format to wmemory_buffer and convert to utf8
                abel::wmemory_buffer wbuf;
                abel::format_to(wbuf, fmt, args...);

                memory_buf_t buf;
                details::os::wstr_to_utf8buf(wstring_view_t(wbuf.data(), wbuf.size()), buf);
                details::log_msg log_msg(loc, name_, lvl, std::string_view(buf.data(), buf.size()));
                log_it_impl(log_msg, log_enabled, traceback_enabled);
            }
            LOG_LOGGER_CATCH()
        }

        // T can be statically converted to wstring_view
        template<class T, typename std::enable_if<is_convertible_to_wstring_view<const T &>::value, int>::type = 0>
        void log(source_loc loc, level::level_enum lvl, const T &msg)
        {
            bool log_enabled = should_log(lvl);
            bool traceback_enabled = tracer_.enabled();
            if (!log_enabled && !traceback_enabled)
            {
                return;
            }

            ABEL_TRY
            {
                memory_buf_t buf;
                details::os::wstr_to_utf8buf(msg, buf);
                details::log_msg log_msg(loc, name_, lvl, std::string_view(buf.data(), buf.size()));
                log_it_impl(log_msg, log_enabled, traceback_enabled);
            }
            LOG_LOGGER_CATCH()
        }
#endif // _WIN32
#endif // LOG_WCHAR_TO_UTF8_SUPPORT

        // return true logging is enabled for the given level.
        bool should_log(level::level_enum msg_level) const {
            return msg_level >= level_.load(std::memory_order_relaxed);
        }

        // return true if backtrace logging is enabled.
        bool should_backtrace() const {
            return tracer_.enabled();
        }

        void set_level(level::level_enum log_level);

        level::level_enum level() const;

        const std::string &name() const;

        // set formatting for the sinks in this logger.
        // each sink will get a separate instance of the formatter object.
        void set_formatter(std::unique_ptr<log_formatter> f);

        void set_pattern(std::string pattern, pattern_time_type time_type = pattern_time_type::local);

        // backtrace support.
        // efficiently store all debug/trace messages in a circular buffer until needed for debugging.
        void enable_backtrace(size_t n_messages);

        void disable_backtrace();

        void dump_backtrace();

        // flush functions
        void flush();

        void flush_on(level::level_enum log_level);

        level::level_enum flush_level() const;

        // sinks
        const std::vector<sink_ptr> &sinks() const;

        std::vector<sink_ptr> &sinks();

        // error handler
        void set_error_handler(err_handler);

        // create new logger with same sinks and configuration.
        virtual std::shared_ptr<logger> clone(std::string logger_name);

    public:
        class log_stream;

        friend class log_stream;

        log_stream stream(level::level_enum lvl, const source_loc &sl);

        log_stream stream(level::level_enum lvl);

        log_stream trace_stream(const source_loc &sl);

        log_stream debug_stream(const source_loc &sl);

        log_stream info_stream();

        log_stream warn_stream();

        log_stream error_stream(const source_loc &sl);

        log_stream critical_stream(const source_loc &sl);

    protected:
        std::string name_;
        std::vector<sink_ptr> sinks_;
        abel::level_t level_{level::info};
        abel::level_t flush_level_{level::off};
        err_handler custom_err_handler_{nullptr};
        details::backtracer tracer_;

        // common implementation for after templated public api has been resolved
        template<typename FormatString, typename... Args>
        void log_impl(source_loc loc, level::level_enum lvl, const FormatString &fmt, const Args &... args) {
            bool log_enabled = should_log(lvl);
            bool traceback_enabled = tracer_.enabled();
            if (!log_enabled && !traceback_enabled) {
                return;
            }
            ABEL_TRY {
                memory_buf_t buf;
                abel::format_to(buf, fmt, args...);
                details::log_msg log_msg(loc, name_, lvl, std::string_view(buf.data(), buf.size()));
                log_it_impl(log_msg, log_enabled, traceback_enabled);
            }
            LOG_LOGGER_CATCH()
        }

        // log the given message (if the given log level is high enough),
        // and save backtrace (if backtrace is enabled).
        void log_it_impl(const details::log_msg &log_msg, bool log_enabled, bool traceback_enabled);

        virtual void sink_it_impl(const details::log_msg &msg);

        virtual void flush_impl();

        void dump_backtrace_();

        bool should_flush_(const details::log_msg &msg);

        // handle errors during logging.
        // default handler prints the error to stderr at max rate of 1 message/sec.
        void err_handler_(const std::string &msg);
    };

    void swap(logger &a, logger &b);

    class logger::log_stream {
    public:
        log_stream(abel::logger *l, level::level_enum lvl, const source_loc &sl)
                : _sl(sl), _logger(l), _lvl(lvl) {

        }

        ~log_stream() {
            if (_logger) {
                details::log_msg log_msg(_sl, _logger->name(), _lvl,
                                         std::string_view(_buf.data(), _buf.size()));
                _logger->log_it_impl(log_msg,
                                     _logger->should_log(_lvl), _logger->tracer_.enabled());
            }
        }

        template<typename T>
        void write(const T &v) {
            abel::format_to(_buf, "{}", v);
        }

        template<typename T>
        log_stream &operator<<(const T &v) {
            write(v);
            return *this;
        }

        template<>
        log_stream &operator<<(const std::string_view &v) {
            write(v);
            return *this;
        }

        template<>
        log_stream &operator<<(const int &v) {
            write(v);
            return *this;
        }

        template<>
        log_stream &operator<<(const char &v) {
            write(v);
            return *this;
        }

        template<>
        log_stream &operator<<(const short &v) {
            write(v);
            return *this;
        }

        template<>
        log_stream &operator<<(const long &v) {
            write(v);
            return *this;
        }

        template<>
        log_stream &operator<<(const long long &v) {
            write(v);
            return *this;
        }

        template<>
        log_stream &operator<<(const float &v) {
            write(v);
            return *this;
        }

        template<>
        log_stream &operator<<(const double &v) {
            write(v);
            return *this;
        }

        template<>
        log_stream &operator<<(const bool &v) {
            v ? write("true") : write("false");
            return *this;
        }

        log_stream &operator<<(const char *str) {
            write(std::string_view(str));
            return *this;
        }


    private:
        abel::memory_buf_t _buf;
        source_loc _sl{};
        abel::logger *_logger;
        level::level_enum _lvl;
    };

    /*template<typename T>
    inline logger::log_stream &operator<<(logger::log_stream &os, const T &v) {
        os.write(v);
    }*/

    inline logger::log_stream logger::stream(level::level_enum lvl, const source_loc &sl) {
        return logger::log_stream(this, lvl, sl);
    }

    inline logger::log_stream logger::stream(level::level_enum lvl) {
        return logger::log_stream(this, lvl, source_loc{});
    }

    inline logger::log_stream logger::trace_stream(const source_loc &sl) {
        return stream(level::trace, sl);
    }

    inline logger::log_stream logger::debug_stream(const source_loc &sl) {
        return stream(level::debug, sl);
    }

    inline logger::log_stream logger::info_stream() {
        return stream(level::info);
    }

    inline logger::log_stream logger::warn_stream() {
        return stream(level::warn);
    }

    inline logger::log_stream logger::error_stream(const source_loc &sl) {
        return stream(level::err, sl);
    }

    inline logger::log_stream logger::critical_stream(const source_loc &sl) {
        return stream(level::critical, sl);
    }
}  // namespace abel

#include "abel/log/logger_inl.h"
