//
// Copyright(c) 2015-2018 Gabi Melman.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//
// abel main header file.
// see example.cpp for usage example

#ifndef ABEL_LOG_LOG_H_
#define ABEL_LOG_LOG_H_

#include <abel/log/common.h>
#include <abel/log/details/registry.h>
#include <abel/log/logger.h>
#include <functional>
#include <memory>
#include <string>

namespace abel {
    namespace log {
// Default logger factory-  creates synchronous loggers
        struct synchronous_factory {
            template<typename Sink, typename... SinkArgs>

            static std::shared_ptr<abel::log::logger> create(std::string logger_name, SinkArgs &&... args) {
                auto sink = std::make_shared<Sink>(std::forward<SinkArgs>(args)...);
                auto new_logger = std::make_shared<logger>(std::move(logger_name), std::move(sink));
                details::registry::instance().register_and_init(new_logger);
                return new_logger;
            }
        };

        using default_factory = synchronous_factory;

// Create and register a logger with a templated sink type
// The logger's level, formatter and flush level will be set according the
// global settings.
// Example:
// abel::create<daily_file_sink_st>("logger_name", "dailylog_filename", 11, 59);
        template<typename Sink, typename... SinkArgs>
        inline std::shared_ptr<abel::log::logger> create(std::string logger_name, SinkArgs &&... sink_args) {
            return default_factory::create<Sink>(std::move(logger_name), std::forward<SinkArgs>(sink_args)...);
        }

// Return an existing logger or nullptr if a logger with such name doesn't
// exist.
// example: abel::get("my_logger")->info("hello {}", "world");
        inline std::shared_ptr<logger> get(const std::string &name) {
            return details::registry::instance().get(name);
        }

// Set global formatter. Each sink in each logger will get a clone of this object
        inline void set_formatter(std::unique_ptr<abel::log::formatter> formatter) {
            details::registry::instance().set_formatter(std::move(formatter));
        }

// Set global format string.
// example: abel::set_pattern("%Y-%m-%d %H:%M:%S.%e %l : %v");
        inline void set_pattern(std::string pattern, pattern_time_type time_type = pattern_time_type::local) {
            set_formatter(std::unique_ptr<abel::log::formatter>(new pattern_formatter(pattern, time_type)));
        }

// Set global logging level
        inline void set_level(level_enum log_level) {
            details::registry::instance().set_level(log_level);
        }

// Set global flush level
        inline void flush_on(level_enum log_level) {
            details::registry::instance().flush_on(log_level);
        }

// Start/Restart a periodic flusher thread
// Warning: Use only if all your loggers are thread safe!
        inline void flush_every(std::chrono::seconds interval) {
            details::registry::instance().flush_every(interval);
        }

// Set global error handler
        inline void set_error_handler(log_err_handler handler) {
            details::registry::instance().set_error_handler(std::move(handler));
        }

// Register the given logger with the given name
        inline void register_logger(std::shared_ptr<logger> logger) {
            details::registry::instance().register_logger(std::move(logger));
        }

// Apply a user defined function on all registered loggers
// Example:
// abel::apply_all([&](std::shared_ptr<abel::logger> l) {l->flush();});
        inline void apply_all(std::function<void(std::shared_ptr<logger>)> fun) {
            details::registry::instance().apply_all(std::move(fun));
        }

// Drop the reference to the given logger
        inline void drop(const std::string &name) {
            details::registry::instance().drop(name);
        }

// Drop all references from the registry
        inline void drop_all() {
            details::registry::instance().drop_all();
        }

// stop any running threads started by abel and clean registry loggers
        inline void shutdown() {
            details::registry::instance().shutdown();
        }

///////////////////////////////////////////////////////////////////////////////
//
// Trace & Debug can be switched on/off at compile time for zero cost debug
// statements.
// Uncomment ABEL_LOG_DEBUG_ON/ABEL_LOG_TRACE_ON in tweakme.h to enable.
// ABEL_LOG_TRACE(..) will also print current file and line.
//
// Example:
// abel::set_level(abel::trace);
// ABEL_LOG_TRACE(my_logger, "some trace message");
// ABEL_LOG_TRACE(my_logger, "another trace message {} {}", 1, 2);
// ABEL_LOG_DEBUG(my_logger, "some debug message {} {}", 3, 4);
///////////////////////////////////////////////////////////////////////////////

#ifdef ABEL_LOG_TRACE_ON

#ifdef _MSC_VER
#define ABEL_LOG_TRACE(logger, ...) logger->trace("[ " __FILE__ "(" ABEL_STRINGIFY(__LINE__) ") ] " __VA_ARGS__)
#else
#define ABEL_LOG_TRACE(logger, ...)                                                                                                          \
        logger->trace("[ " __FILE__ ":" ABEL_STRINGIFY(__LINE__) " ]"                                                                       \
                                                                    " " __VA_ARGS__)
#endif
#else
#define ABEL_LOG_TRACE(logger, ...) (void)0
#endif

#ifdef ABEL_LOG_DEBUG_ON
#define ABEL_LOG_DEBUG(logger, ...) logger->debug(__VA_ARGS__)
#else
#define ABEL_LOG_DEBUG(logger, ...) (void)0
#endif
    }
} // namespace abel
#endif //ABEL_LOG_LOG_H_
