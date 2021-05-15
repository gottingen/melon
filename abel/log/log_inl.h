
#pragma once


#include "abel/log/common.h"
#include "abel/log/pattern_formatter.h"

namespace abel {

    ABEL_FORCE_INLINE void initialize_logger(std::shared_ptr<logger> logger) {
        details::registry::instance().initialize_logger(std::move(logger));
    }

    ABEL_FORCE_INLINE std::shared_ptr<logger> get(const std::string &name) {
        return details::registry::instance().get(name);
    }

    ABEL_FORCE_INLINE void set_formatter(std::unique_ptr<abel::log_formatter> formatter) {
        details::registry::instance().set_formatter(std::move(formatter));
    }

    ABEL_FORCE_INLINE void set_pattern(std::string pattern, pattern_time_type time_type) {
        set_formatter(std::unique_ptr<abel::log_formatter>(new pattern_formatter(std::move(pattern), time_type)));
    }

    ABEL_FORCE_INLINE void enable_backtrace(size_t n_messages) {
        details::registry::instance().enable_backtrace(n_messages);
    }

    ABEL_FORCE_INLINE void disable_backtrace() {
        details::registry::instance().disable_backtrace();
    }

    ABEL_FORCE_INLINE void dump_backtrace() {
        default_logger_raw()->dump_backtrace();
    }

    ABEL_FORCE_INLINE void set_level(level::level_enum log_level) {
        details::registry::instance().set_level(log_level);
    }

    ABEL_FORCE_INLINE void flush_on(level::level_enum log_level) {
        details::registry::instance().flush_on(log_level);
    }

    ABEL_FORCE_INLINE void flush_every(std::chrono::seconds interval) {
        details::registry::instance().flush_every(interval);
    }

    ABEL_FORCE_INLINE void set_error_handler(void (*handler)(const std::string &msg)) {
        details::registry::instance().set_error_handler(handler);
    }

    ABEL_FORCE_INLINE void register_logger(std::shared_ptr<logger> logger) {
        details::registry::instance().register_logger(std::move(logger));
    }

    ABEL_FORCE_INLINE void apply_all(const std::function<void(std::shared_ptr<logger>)> &fun) {
        details::registry::instance().apply_all(fun);
    }

    ABEL_FORCE_INLINE void drop(const std::string &name) {
        details::registry::instance().drop(name);
    }

    ABEL_FORCE_INLINE void drop_all() {
        details::registry::instance().drop_all();
    }

    ABEL_FORCE_INLINE void shutdown() {
        details::registry::instance().shutdown();
    }

    ABEL_FORCE_INLINE void set_automatic_registration(bool automatic_registration) {
        details::registry::instance().set_automatic_registration(automatic_registration);
    }

    ABEL_FORCE_INLINE std::shared_ptr<abel::logger> default_logger() {
        return details::registry::instance().default_logger();
    }

    ABEL_FORCE_INLINE abel::logger *default_logger_raw() {
        return details::registry::instance().get_default_raw();
    }

    ABEL_FORCE_INLINE void set_default_logger(std::shared_ptr<abel::logger> default_logger) {
        details::registry::instance().set_default_logger(std::move(default_logger));
    }

}  // namespace abel

