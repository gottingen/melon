// Copyright(c) 2015-present, Gabi Melman & spdlog contributors.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

// spdlog main header file.
// see example.cpp for usage example

#ifndef LOG_H
#define LOG_H

#pragma once
#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include "abel/log/common.h"
#include "abel/log/details/registry.h"
#include "abel/log/logger.h"
#include "abel/log/version.h"
#include "abel/log/details/synchronous_factory.h"


namespace abel {

using default_factory = synchronous_factory;

// Create and register a logger with a templated sink type
// The logger's level, log_formatter and flush level will be set according the
// global settings.
//
// Example:
//   abel::create<daily_file_sink_st>("logger_name", "dailylog_filename", 11, 59);
template<typename Sink, typename... SinkArgs>
inline std::shared_ptr<abel::logger> create(std::string logger_name, SinkArgs &&... sink_args) {
    return default_factory::create<Sink>(std::move(logger_name), std::forward<SinkArgs>(sink_args)...);
}

// Initialize and register a logger,
// log_formatter and flush level will be set according the global settings.
//
// Useful for initializing manually created loggers with the global settings.
//
// Example:
//   auto mylogger = std::make_shared<abel::logger>("mylogger", ...);
//   abel::initialize_logger(mylogger);
ABEL_API void initialize_logger(std::shared_ptr<logger> logger);

// Return an existing logger or nullptr if a logger with such name doesn't
// exist.
// example: abel::get("my_logger")->info("hello {}", "world");
ABEL_API std::shared_ptr<logger> get(const std::string &name);

// Set global log_formatter. Each sink in each logger will get a clone of this object
ABEL_API void set_formatter(std::unique_ptr<abel::log_formatter> formatter);

// Set global format string.
// example: abel::set_pattern("%Y-%m-%d %H:%M:%S.%e %l : %v");
ABEL_API void set_pattern(std::string pattern, pattern_time_type time_type = pattern_time_type::local);

// enable global backtrace support
ABEL_API void enable_backtrace(size_t n_messages);

// disable global backtrace support
ABEL_API void disable_backtrace();

// call dump backtrace on default logger
ABEL_API void dump_backtrace();

// Set global logging level
ABEL_API void set_level(level::level_enum log_level);

// Set global flush level
ABEL_API void flush_on(level::level_enum log_level);

// Start/Restart a periodic flusher thread
// Warning: Use only if all your loggers are thread safe!
ABEL_API void flush_every(std::chrono::seconds interval);

// Set global error handler
ABEL_API void set_error_handler(void (*handler)(const std::string &msg));

// Register the given logger with the given name
ABEL_API void register_logger(std::shared_ptr<logger> logger);

// Apply a user defined function on all registered loggers
// Example:
// abel::apply_all([&](std::shared_ptr<abel::logger> l) {l->flush();});
ABEL_API void apply_all(const std::function<void(std::shared_ptr<logger>)> &fun);

// Drop the reference to the given logger
ABEL_API void drop(const std::string &name);

// Drop all references from the registry
ABEL_API void drop_all();

// stop any running threads started by spdlog and clean registry loggers
ABEL_API void shutdown();

// Automatic registration of loggers when using abel::create() or abel::create_async
ABEL_API void set_automatic_registration(bool automatic_registration);

// API for using default logger (stdout_color_mt),
// e.g: abel::info("Message {}", 1);
//
// The default logger object can be accessed using the abel::default_logger():
// For example, to add another sink to it:
// abel::default_logger()->sinks()->push_back(some_sink);
//
// The default logger can replaced using abel::set_default_logger(new_logger).
// For example, to replace it with a file logger.
//
// IMPORTANT:
// The default API is thread safe (for _mt loggers), but:
// set_default_logger() *should not* be used concurrently with the default API.
// e.g do not call set_default_logger() from one thread while calling abel::info() from another.

ABEL_API std::shared_ptr<abel::logger> default_logger();

ABEL_API abel::logger *default_logger_raw();

ABEL_API void set_default_logger(std::shared_ptr<abel::logger> default_logger);

template<typename FormatString, typename... Args>
inline void log(source_loc source, level::level_enum lvl, const FormatString &fmt, const Args &... args) {
    default_logger_raw()->log(source, lvl, fmt, args...);
}

template<typename FormatString, typename... Args>
inline void log(level::level_enum lvl, const FormatString &fmt, const Args &... args) {
    default_logger_raw()->log(source_loc{}, lvl, fmt, args...);
}

template<typename FormatString, typename... Args>
inline void trace(const FormatString &fmt, const Args &... args) {
    default_logger_raw()->trace(fmt, args...);
}

template<typename FormatString, typename... Args>
inline void debug(const FormatString &fmt, const Args &... args) {
    default_logger_raw()->debug(fmt, args...);
}

template<typename FormatString, typename... Args>
inline void info(const FormatString &fmt, const Args &... args) {
    default_logger_raw()->info(fmt, args...);
}

template<typename FormatString, typename... Args>
inline void warn(const FormatString &fmt, const Args &... args) {
    default_logger_raw()->warn(fmt, args...);
}

template<typename FormatString, typename... Args>
inline void error(const FormatString &fmt, const Args &... args) {
    default_logger_raw()->error(fmt, args...);
}

template<typename FormatString, typename... Args>
inline void critical(const FormatString &fmt, const Args &... args) {
    default_logger_raw()->critical(fmt, args...);
}

template<typename T>
inline void log(source_loc source, level::level_enum lvl, const T &msg) {
    default_logger_raw()->log(source, lvl, msg);
}

template<typename T>
inline void log(level::level_enum lvl, const T &msg) {
    default_logger_raw()->log(lvl, msg);
}

template<typename T>
inline void trace(const T &msg) {
    default_logger_raw()->trace(msg);
}

template<typename T>
inline void debug(const T &msg) {
    default_logger_raw()->debug(msg);
}

template<typename T>
inline void info(const T &msg) {
    default_logger_raw()->info(msg);
}

template<typename T>
inline void warn(const T &msg) {
    default_logger_raw()->warn(msg);
}

template<typename T>
inline void error(const T &msg) {
    default_logger_raw()->error(msg);
}

template<typename T>
inline void critical(const T &msg) {
    default_logger_raw()->critical(msg);
}

}  // namespace abel

#include "abel/log/log_inl.h"

#endif // LOG_H
