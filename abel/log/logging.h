//
// Created by liyinbin on 2020/7/31.
//

#ifndef ABEL_LOG_LOGGING_H_
#define ABEL_LOG_LOGGING_H_

#include "abel/log/log.h"
#include "abel/utility/every.h"
#include "abel/chrono/clock.h"
#include "abel/log/common.h"

//Introduction
//
//


//
// enable/disable log calls at compile time according to global level.
//
// define LOG_ACTIVE_LEVEL to one of those (before including logging.h):
// LOG_LEVEL_TRACE,
// LOG_LEVEL_DEBUG,
// LOG_LEVEL_INFO,
// LOG_LEVEL_WARN,
// LOG_LEVEL_ERROR,
// LOG_LEVEL_CRITICAL,
// LOG_LEVEL_OFF
//

struct every_second {
    every_second() {

    }

    bool feed() {
        int64_t sec = abel::time_now().to_unix_seconds();
        return sec > epoch_second ? epoch_second.compare_exchange_weak(sec, std::memory_order_relaxed) : false;
    }

    std::atomic<int64_t> epoch_second{0};
};

#define LOG_CALL(logger, level, ...) (logger)->log(abel::source_loc{__FILE__, __LINE__, ABEL_PRETTY_FUNCTION}, level, __VA_ARGS__)
#define LOG_CALL_IF(logger, level, condition, ...) !(condition) ? (void)0 : LOG_CALL(logger, level, __VA_ARGS__)

#define LOG_CALL_IF_EVERY_N(logger, level, condition, N, ...) \
    static abel::every_n ABEL_CONCAT(log_every_n, __LINE__)(N); \
    LOG_CALL_IF(logger, level, (condition)&ABEL_CONCAT(log_every_n, __LINE__).feed(), __VA_ARGS__)

#define LOG_CALL_IF_FIRST_N(logger, level, condition, N, ...) \
    static abel::first_n ABEL_CONCAT(log_every_n, __LINE__)(N); \
    LOG_CALL_IF(logger, level, (condition)&ABEL_CONCAT(log_every_n, __LINE__).feed(), __VA_ARGS__)

#define LOG_CALL_IF_EVERY_SECOND(logger, level, condition, ...) \
    static abel::every_second ABEL_CONCAT(log_every_n, __LINE__)(N); \
    LOG_CALL_IF(logger, level, (condition)&ABEL_CONCAT(log_every_n, __LINE__).feed(), __VA_ARGS__)


#if LOG_ACTIVE_LEVEL <= LOG_LEVEL_TRACE
#define LOG_TRACE(logger, ...) LOG_CALL(logger, abel::level::trace, __VA_ARGS__)
#define LOG_TRACE_IF(logger, condition, ...) LOG_CALL_IF(logger, abel::level::trace, condition, __VA_ARGS__)
#define LOG_TRACE_EVERY_N(logger, N, ...)  LOG_CALL_IF_EVERY_N(logger, abel::level::trace, true, N, __VA_ARGS__)
#define LOG_TRACE_EVERY_N_IF(logger, condition, N, ...)  LOG_CALL_IF_EVERY_N(logger, abel::level::trace, condition, N, __VA_ARGS__)
#define LOG_TRACE_FIRST_N(logger, N, ...) LOG_CALL_IF_FIRST_N(logger, abel::level::trace, true, __VA_ARGS__)
#define LOG_TRACE_FIRST_N_IF(logger, condition, N, ...) LOG_CALL_IF_FIRST_N(logger, abel::level::trace, true, __VA_ARGS__)
#define LOG_TRACE_EVERY_SECOND(logger, ...) LOG_CALL_IF_EVERY_SECOND(logger, abel::level::trace, true, __VA_ARGS__)
#define LOG_TRACE_EVERY_SECOND_IF(logger, condition, ...) LOG_CALL_IF_EVERY_SECOND(logger, abel::level::trace, condition, __VA_ARGS__)
// default log
#define DLOG_TRACE(...) LOG_TRACE(abel::default_logger_raw(), __VA_ARGS__)
#define DLOG_TRACE_IF(condition, ...) LOG_TRACE_IF(abel::default_logger_raw(), condition, __VA_ARGS__)
#define DLOG_TRACE_EVERY_N(N, ...) LOG_TRACE_EVERY_N(abel::default_logger_raw(), N, __VA_ARGS__)
#define DLOG_TRACE_EVERY_N_IF(condition, N, ...) LOG_TRACE_EVERY_N_IF(abel::default_logger_raw(), condition, N, __VA_ARGS__)
#define DLOG_TRACE_FIRST_N(N, ...) LOG_TRACE_FIRST_N(abel::default_logger_raw(), N, __VA_ARGS__)
#define DLOG_TRACE_FIRST_N_IF(condition, N, ...) LOG_TRACE_FIRST_N_IF(abel::default_logger_raw(), condition, N, __VA_ARGS__)
#define DLOG_TRACE_EVERY_SECOND(...) LOG_TRACE_EVERY_SECOND(abel::default_logger_raw(), __VA_ARGS__)
#define DLOG_TRACE_EVERY_SECOND_IF(condition, ...) LOG_TRACE_EVERY_SECOND_IF(abel::default_logger_raw(),condition, __VA_ARGS__)

#else
#define LOG_TRACE(logger, ...) (void)0
#define LOG_TRACE_IF(logger, condition, ...) (void)0
#define LOG_TRACE_EVERY_N(logger, N, ...)  (void)0
#define LOG_TRACE_EVERY_N_IF(logger, condition, N, ...)  (void)0
#define LOG_TRACE_FIRST_N(logger, N, ...) (void)0
#define LOG_TRACE_FIRST_N_IF(logger, condition, N, ...) (void)0
#define LOG_TRACE_EVERY_SECOND(logger, ...) (void)0
#define LOG_TRACE_EVERY_SECOND_IF(logger, condition, ...) (void)0
// default log
#define DLOG_TRACE(...) (void)0
#define DLOG_TRACE_IF(condition, ...) (void)0
#define DLOG_TRACE_EVERY_N(N, ...) (void)0
#define DLOG_TRACE_EVERY_N_IF(condition, N, ...) (void)0
#define DLOG_TRACE_FIRST_N(N, ...) (void)0
#define DLOG_TRACE_FIRST_N_IF(condition, N, ...) (void)0
#define DLOG_TRACE_EVERY_SECOND(...) (void)0
#define DLOG_TRACE_EVERY_SECOND_IF(condition, ...) (void)0


#endif

#if LOG_ACTIVE_LEVEL <= LOG_LEVEL_DEBUG
#define LOG_DEBUG(logger, ...) LOG_CALL(logger, abel::level::debug, __VA_ARGS__)
#define LOG_DEBUG_IF(logger, condition, ...) LOG_CALL_IF(logger, abel::level::debug, condition, __VA_ARGS__)
#define LOG_DEBUG_EVERY_N(logger, N, ...)  LOG_CALL_IF_EVERY_N(logger, abel::level::debug, true, N, __VA_ARGS__)
#define LOG_DEBUG_EVERY_N_IF(logger, condition, N, ...)  LOG_CALL_IF_EVERY_N(logger, abel::level::debug, condition, N, __VA_ARGS__)
#define LOG_DEBUG_FIRST_N(logger, N, ...) LOG_CALL_IF_FIRST_N(logger, abel::level::debug, true, __VA_ARGS__)
#define LOG_DEBUG_FIRST_N_IF(logger, condition, N, ...) LOG_CALL_IF_FIRST_N(logger, abel::level::debug, true, __VA_ARGS__)
#define LOG_DEBUG_EVERY_SECOND(logger, ...) LOG_CALL_IF_EVERY_SECOND(logger, abel::level::debug, true, __VA_ARGS__)
#define LOG_DEBUG_EVERY_SECOND_IF(logger, condition, ...) LOG_CALL_IF_EVERY_SECOND(logger, abel::level::debug, condition, __VA_ARGS__)
// default log
#define DLOG_DEBUG(...) LOG_DEBUG(abel::default_logger_raw(), __VA_ARGS__)
#define DLOG_DEBUG_IF(condition, ...) LOG_DEBUG_IF(abel::default_logger_raw(), condition, __VA_ARGS__)
#define DLOG_DEBUG_EVERY_N(N, ...) LOG_DEBUG_EVERY_N(abel::default_logger_raw(), N, __VA_ARGS__)
#define DLOG_DEBUG_EVERY_N_IF(condition, N, ...) LOG_DEBUG_EVERY_N_IF(abel::default_logger_raw(), condition, N, __VA_ARGS__)
#define DLOG_DEBUG_FIRST_N(N, ...) LOG_DEBUG_FIRST_N(abel::default_logger_raw(), N, __VA_ARGS__)
#define DLOG_DEBUG_FIRST_N_IF(condition, N, ...) LOG_DEBUG_FIRST_N_IF(abel::default_logger_raw(), condition, N, __VA_ARGS__)
#define DLOG_DEBUG_EVERY_SECOND(...) LOG_DEBUG_EVERY_SECOND(abel::default_logger_raw(), __VA_ARGS__)
#define DLOG_DEBUG_EVERY_SECOND_IF(condition, ...) LOG_DEBUG_EVERY_SECOND_IF(abel::default_logger_raw(),condition, __VA_ARGS__)

#else
#define LOG_DEBUG(logger, ...) (void)0
#define LOG_DEBUG_IF(logger, condition, ...) (void)0
#define LOG_DEBUG_EVERY_N(logger, N, ...)  (void)0
#define LOG_DEBUG_EVERY_N_IF(logger, condition, N, ...)  (void)0
#define LOG_DEBUG_FIRST_N(logger, N, ...) (void)0
#define LOG_DEBUG_FIRST_N_IF(logger, condition, N, ...) (void)0
#define LOG_DEBUG_EVERY_SECOND(logger, ...) (void)0
#define LOG_DEBUG_EVERY_SECOND_IF(logger, condition, ...) (void)0
// default log
#define DLOG_DEBUG(...) (void)0
#define DLOG_DEBUG_IF(condition, ...) (void)0
#define DLOG_DEBUG_EVERY_N(N, ...) (void)0
#define DLOG_DEBUG_EVERY_N_IF(condition, N, ...) (void)0
#define DLOG_DEBUG_FIRST_N(N, ...) (void)0
#define DLOG_DEBUG_FIRST_N_IF(condition, N, ...) (void)0
#define DLOG_DEBUG_EVERY_SECOND(...) (void)0
#define DLOG_DEBUG_EVERY_SECOND_IF(condition, ...) (void)0


#endif

#if LOG_ACTIVE_LEVEL <= LOG_LEVEL_INFO
#define LOG_INFO(logger, ...) LOG_CALL(logger, abel::level::info, __VA_ARGS__)
#define LOG_INFO_IF(logger, condition, ...) LOG_CALL_IF(logger, abel::level::info, condition, __VA_ARGS__)
#define LOG_INFO_EVERY_N(logger, N, ...)  LOG_CALL_IF_EVERY_N(logger, abel::level::info, true, N, __VA_ARGS__)
#define LOG_INFO_EVERY_N_IF(logger, condition, N, ...)  LOG_CALL_IF_EVERY_N(logger, abel::level::info, condition, N, __VA_ARGS__)
#define LOG_INFO_FIRST_N(logger, N, ...) LOG_CALL_IF_FIRST_N(logger, abel::level::info, true, __VA_ARGS__)
#define LOG_INFO_FIRST_N_IF(logger, condition, N, ...) LOG_CALL_IF_FIRST_N(logger, abel::level::info, true, __VA_ARGS__)
#define LOG_INFO_EVERY_SECOND(logger, ...) LOG_CALL_IF_EVERY_SECOND(logger, abel::level::info, true, __VA_ARGS__)
#define LOG_INO_EVERY_SECOND_IF(logger, condition, ...) LOG_CALL_IF_EVERY_SECOND(logger, abel::level::info, condition, __VA_ARGS__)
// default log
#define DLOG_INFO(...) LOG_INFO(abel::default_logger_raw(), __VA_ARGS__)
#define DLOG_INFO_IF(condition, ...) LOG_INFO_IF(abel::default_logger_raw(), condition, __VA_ARGS__)
#define DLOG_INFO_EVERY_N(N, ...) LOG_INFO_EVERY_N(abel::default_logger_raw(), N, __VA_ARGS__)
#define DLOG_INFO_EVERY_N_IF(condition, N, ...) LOG_INFO_EVERY_N_IF(abel::default_logger_raw(), condition, N, __VA_ARGS__)
#define DLOG_INFO_FIRST_N(N, ...) LOG_INFO_FIRST_N(abel::default_logger_raw(), N, __VA_ARGS__)
#define DLOG_INFO_FIRST_N_IF(condition, N, ...) LOG_INFO_FIRST_N_IF(abel::default_logger_raw(), condition, N, __VA_ARGS__)
#define DLOG_INFO_EVERY_SECOND(...) LOG_INFO_EVERY_SECOND(abel::default_logger_raw(), __VA_ARGS__)
#define DLOG_INO_EVERY_SECOND_IF(condition, ...) LOG_INO_EVERY_SECOND_IF(abel::default_logger_raw(),condition, __VA_ARGS__)

#else
#define LOG_INFO(logger, ...) (void)0
#define LOG_INFO_IF(logger, condition, ...) (void)0
#define LOG_INFO_EVERY_N(logger, N, ...) (void)0
#define LOG_INFO_EVERY_N_IF(logger, condition, N, ...)  (void)0
#define LOG_INFO_FIRST_N(logger, N, ...) (void)0
#define LOG_INFO_FIRST_N_IF(logger, condition, N, ...) (void)0
#define LOG_INFO_EVERY_SECOND(logger, ...) (void)0
#define LOG_INO_EVERY_SECOND_IF(logger, condition, ...) (void)0
// default log
#define DLOG_INFO(...) (void)0
#define DLOG_INFO_IF(condition, ...) (void)0
#define DLOG_INFO_EVERY_N(N, ...) (void)0
#define DLOG_INFO_EVERY_N_IF(condition, N, ...) (void)0
#define DLOG_INFO_FIRST_N(N, ...) (void)0
#define DLOG_INFO_FIRST_N_IF(condition, N, ...) (void)0
#define DLOG_INFO_EVERY_SECOND(...) (void)0
#define DLOG_INO_EVERY_SECOND_IF(condition, ...) (void)0
#endif

#if LOG_ACTIVE_LEVEL <= LOG_LEVEL_WARN
#define LOG_WARN(logger, ...) LOG_CALL(logger, abel::level::warn, __VA_ARGS__)
#define LOG_WARN_IF(logger, condition, ...) LOG_CALL_IF(logger, abel::level::warn, condition, __VA_ARGS__)
#define LOG_WARN_EVERY_N(logger, N, ...)  LOG_CALL_IF_EVERY_N(logger, abel::level::warn, true, N, __VA_ARGS__)
#define LOG_WARN_EVERY_N_IF(logger, condition, N, ...)  LOG_CALL_IF_EVERY_N(logger, abel::level::warn, condition, N, __VA_ARGS__)
#define LOG_WARN_FIRST_N(logger, N, ...) LOG_CALL_IF_FIRST_N(logger, abel::level::warn, true, __VA_ARGS__)
#define LOG_WARN_FIRST_N_IF(logger, condition, N, ...) LOG_CALL_IF_FIRST_N(logger, abel::level::warn, true, __VA_ARGS__)
#define LOG_WARN_EVERY_SECOND(logger, ...) LOG_CALL_IF_EVERY_SECOND(logger, abel::level::warn, true, __VA_ARGS__)
#define LOG_WARN_EVERY_SECOND_IF(logger, condition, ...) LOG_CALL_IF_EVERY_SECOND(logger, abel::level::warn, condition, __VA_ARGS__)
// default log
#define DLOG_WARN(...) LOG_WARN(abel::default_logger_raw(), __VA_ARGS__)
#define DLOG_WARN_IF(condition, ...) LOG_WARN_IF(abel::default_logger_raw(), condition, __VA_ARGS__)
#define DLOG_WARN_EVERY_N(N, ...) LOG_WARN_EVERY_N(abel::default_logger_raw(), N, __VA_ARGS__)
#define DLOG_WARN_EVERY_N_IF(condition, N, ...) LOG_WARN_EVERY_N_IF(abel::default_logger_raw(), condition, N, __VA_ARGS__)
#define DLOG_WARN_FIRST_N(N, ...) LOG_WARN_FIRST_N(abel::default_logger_raw(), N, __VA_ARGS__)
#define DLOG_WARN_FIRST_N_IF(condition, N, ...) LOG_WARN_FIRST_N_IF(abel::default_logger_raw(), condition, N, __VA_ARGS__)
#define DLOG_WARN_EVERY_SECOND(...) LOG_WARN_EVERY_SECOND(abel::default_logger_raw(), __VA_ARGS__)
#define DLOG_WARN_EVERY_SECOND_IF(condition, ...) LOG_WARN_EVERY_SECOND_IF(abel::default_logger_raw(),condition, __VA_ARGS__)

#else
#define LOG_WARN(logger, ...) (void)0
#define LOG_WARN_IF(logger, condition, ...) (void)0
#define LOG_WARN_EVERY_N(logger, N, ...)  (void)0
#define LOG_WARN_EVERY_N_IF(logger, condition, N, ...)  (void)0
#define LOG_WARN_FIRST_N(logger, N, ...) (void)0
#define LOG_WARN_FIRST_N_IF(logger, condition, N, ...) (void)0
#define LOG_WARN_EVERY_SECOND(logger, ...) (void)0
#define LOG_WARN_EVERY_SECOND_IF(logger, condition, ...) (void)0
// default log
#define DLOG_WARN(...) (void)0
#define DLOG_WARN_IF(condition, ...) (void)0
#define DLOG_WARN_EVERY_N(N, ...) (void)0
#define DLOG_WARN_EVERY_N_IF(condition, N, ...) (void)0
#define DLOG_WARN_FIRST_N(N, ...) (void)0
#define DLOG_WARN_FIRST_N_IF(condition, N, ...) (void)0
#define DLOG_WARN_EVERY_SECOND(...) (void)0
#define DLOG_WARN_EVERY_SECOND_IF(condition, ...) (void)0
#endif

#if LOG_ACTIVE_LEVEL <= LOG_LEVEL_ERROR

#define LOG_ERROR(logger, ...) LOG_CALL(logger, abel::level::err, __VA_ARGS__)
#define LOG_ERROR_IF(logger, condition, ...) LOG_CALL_IF(logger, abel::level::err, condition, __VA_ARGS__)
#define LOG_ERROR_EVERY_N(logger, N, ...)  LOG_CALL_IF_EVERY_N(logger, abel::level::err, true, N, __VA_ARGS__)
#define LOG_ERROR_EVERY_N_IF(logger, condition, N, ...)  LOG_CALL_IF_EVERY_N(logger, abel::level::err, condition, N, __VA_ARGS__)
#define LOG_ERROR_FIRST_N(logger, N, ...) LOG_CALL_IF_FIRST_N(logger, abel::level::err, true, __VA_ARGS__)
#define LOG_ERROR_FIRST_N_IF(logger, condition, N, ...) LOG_CALL_IF_FIRST_N(logger, abel::level::err, true, __VA_ARGS__)
#define LOG_ERROR_EVERY_SECOND(logger, ...) LOG_CALL_IF_EVERY_SECOND(logger, abel::level::err, true, __VA_ARGS__)
#define LOG_ERROR_EVERY_SECOND_IF(logger, condition, ...) LOG_CALL_IF_EVERY_SECOND(logger, abel::level::err, condition, __VA_ARGS__)
// default log
#define DLOG_ERROR(...) LOG_ERROR(abel::default_logger_raw(), __VA_ARGS__)
#define DLOG_ERROR_IF(condition, ...) LOG_ERROR_IF(abel::default_logger_raw(), condition, __VA_ARGS__)
#define DLOG_ERROR_EVERY_N(N, ...) LOG_ERROR_EVERY_N(abel::default_logger_raw(), N, __VA_ARGS__)
#define DLOG_ERROR_EVERY_N_IF(condition, N, ...) LOG_ERROR_EVERY_N_IF(abel::default_logger_raw(), condition, N, __VA_ARGS__)
#define DLOG_ERROR_FIRST_N(N, ...) LOG_ERROR_FIRST_N(abel::default_logger_raw(), N, __VA_ARGS__)
#define DLOG_ERROR_FIRST_N_IF(condition, N, ...) LOG_ERROR_FIRST_N_IF(abel::default_logger_raw(), condition, N, __VA_ARGS__)
#define DLOG_ERROR_EVERY_SECOND(...) LOG_ERROR_EVERY_SECOND(abel::default_logger_raw(), __VA_ARGS__)
#define DLOG_ERROR_EVERY_SECOND_IF(condition, ...) LOG_ERROR_EVERY_SECOND_IF(abel::default_logger_raw(),condition, __VA_ARGS__)

#else
#define LOG_ERROR(logger, ...) (void)0
#define LOG_ERROR_IF(logger, condition, ...) (void)0
#define LOG_ERROR_EVERY_N(logger, N, ...)  (void)0
#define LOG_ERROR_EVERY_N_IF(logger, condition, N, ...)  (void)0
#define LOG_ERROR_FIRST_N(logger, N, ...) (void)0
#define LOG_ERROR_FIRST_N_IF(logger, condition, N, ...) (void)0
#define LOG_ERROR_EVERY_SECOND(logger, ...) (void)0
#define LOG_ERROR_EVERY_SECOND_IF(logger, condition, ...) (void)0
// default log
#define DLOG_ERROR(...) (void)0
#define DLOG_ERROR_IF(condition, ...) (void)0
#define DLOG_ERROR_EVERY_N(N, ...) (void)0
#define DLOG_ERROR_EVERY_N_IF(condition, N, ...) (void)0
#define DLOG_ERROR_FIRST_N(N, ...) (void)0
#define DLOG_ERROR_FIRST_N_IF(condition, N, ...) (void)0
#define DLOG_ERROR_EVERY_SECOND(...) (void)0
#define DLOG_ERROR_EVERY_SECOND_IF(condition, ...) (void)0

#endif

#if LOG_ACTIVE_LEVEL <= LOG_LEVEL_CRITICAL
#define LOG_CRITICAL(logger, ...) LOG_CALL(logger, abel::level::critical, __VA_ARGS__)
#define LOG_CRITICAL_IF(logger, condition, ...) LOG_CALL_IF(logger, abel::level::critical, condition, __VA_ARGS__)
#define LOG_CRITICAL_EVERY_N(logger, N, ...)  LOG_CALL_IF_EVERY_N(logger, abel::level::critical, true, N, __VA_ARGS__)
#define LOG__CRITICAL_EVERY_N_IF(logger, condition, N, ...)  LOG_CALL_IF_EVERY_N(logger, abel::level::critical, condition, N, __VA_ARGS__)
#define LOG_CRITICAL_FIRST_N(logger, N, ...) LOG_CALL_IF_FIRST_N(logger, abel::level::critical, true, __VA_ARGS__)
#define LOG_CRITICAL_FIRST_N_IF(logger, condition, N, ...) LOG_CALL_IF_FIRST_N(logger, abel::level::critical, true, __VA_ARGS__)
#define LOG_CRITICAL_EVERY_SECOND(logger, ...) LOG_CALL_IF_EVERY_SECOND(logger, abel::level::critical, true, __VA_ARGS__)
#define LOG_CRITICAL_EVERY_SECOND_IF(logger, condition, ...) LOG_CALL_IF_EVERY_SECOND(logger, abel::level::critical, condition, __VA_ARGS__)
// default log
#define DLOG_CRITICAL(...) LOG_CRITICAL(abel::default_logger_raw(), __VA_ARGS__)
#define DLOG_CRITICAL_IF(condition, ...) LOG_CRITICAL_IF(abel::default_logger_raw(), condition, __VA_ARGS__)
#define DLOG_CRITICAL_EVERY_N(N, ...) LOG_CRITICAL_EVERY_N(abel::default_logger_raw(), N, __VA_ARGS__)
#define DLOG__CRITICAL_EVERY_N_IF(condition, N, ...) LOG__CRITICAL_EVERY_N_IF(abel::default_logger_raw(), condition, N, __VA_ARGS__)
#define DLOG_CRITICAL_FIRST_N(N, ...) LOG_CRITICAL_FIRST_N(abel::default_logger_raw(), N, __VA_ARGS__)
#define DLOG_CRITICAL_FIRST_N_IF(condition, N, ...) LOG_CRITICAL_FIRST_N_IF(abel::default_logger_raw(), condition, N, __VA_ARGS__)
#define DLOG_CRITICAL_EVERY_SECOND(...) LOG_CRITICAL_EVERY_SECOND(abel::default_logger_raw(), __VA_ARGS__)
#define DLOG_CRITICAL_EVERY_SECOND_IF(condition, ...) LOG_CRITICAL_EVERY_SECOND_IF(abel::default_logger_raw(),condition, __VA_ARGS__)

#else
#define LOG_CRITICAL(logger, ...) (void)0
#define LOG_CRITICAL_IF(logger, condition, ...) (void)0
#define LOG_CRITICAL_EVERY_N(logger, N, ...)  (void)0
#define LOG__CRITICAL_EVERY_N_IF(logger, condition, N, ...)  (void)0
#define LOG_CRITICAL_FIRST_N(logger, N, ...) (void)0
#define LOG_CRITICAL_FIRST_N_IF(logger, condition, N, ...) (void)0
#define LOG_CRITICAL_EVERY_SECOND(logger, ...) (void)0
#define LOG_CRITICAL_EVERY_SECOND_IF(logger, condition, ...) (void)0
// default log
#define DLOG_CRITICAL(...) (void)0
#define DLOG_CRITICAL_IF(condition, ...) (void)0
#define DLOG_CRITICAL_EVERY_N(N, ...) (void)0
#define DLOG__CRITICAL_EVERY_N_IF(condition, N, ...) (void)0
#define DLOG_CRITICAL_FIRST_N(N, ...) (void)0
#define DLOG_CRITICAL_FIRST_N_IF(condition, N, ...) (void)0
#define DLOG_CRITICAL_EVERY_SECOND(...) (void)0
#define DLOG_CRITICAL_EVERY_SECOND_IF(condition, ...) (void)0

#endif

/// stream log apis


namespace log_internal {
    template<class... Ts>
    std::string format_log_msg(const Ts &... args) noexcept {
        std::string result;

        if constexpr (sizeof...(Ts) != 0) {
            result += abel::format(args...);
        }
        return result;
    }
} // namespace log_internal

#define ABEL_INTERNAL_LOGGING_CHECK(logger, expr, ...)                        \
    do {                                                                        \
        if (ABEL_UNLIKELY(!(expr))) {                                            \
            logger->critical_stream(abel::source_loc(__FILE__, __LINE__, __FUNCTION__))      \
                << "Check failed: " #expr " "                                       \
                <<log_internal::format_log_msg(__VA_ARGS__);            \
                abort();                                                    \
        }                                                                         \
    } while (0)

#define ABEL_INTERNAL_LOGGING_CHECK_OP(logger, name, op, val1, val2, ...)      \
    do {                                                                         \
            auto&& abel_anonymous_x = (val1);                                         \
            auto&& abel_anonymous_y = (val2);                                         \
            if (ABEL_UNLIKELY(!(abel_anonymous_x op abel_anonymous_y))) {           \
                logger->stream(abel::level::critical, abel::source_loc(__FILE__, __LINE__, __FUNCTION__))      \
                << log_internal::format_log_msg(__VA_ARGS__);             \
                abort();                                                     \
            }                                                                          \
    } while (0)

#define CHECK(expr, ...) \
    ABEL_INTERNAL_LOGGING_CHECK(abel::default_logger_raw(), expr, __VA_ARGS__)

#define CHECK_EQ(val1, val2, ...) \
    ABEL_INTERNAL_LOGGING_CHECK_OP(abel::default_logger_raw(), EQ_, ==, val1, val2, __VA_ARGS__)

#define CHECK_NE(val1, val2, ...) \
    ABEL_INTERNAL_LOGGING_CHECK_OP(abel::default_logger_raw(), NE_, !=, val1, val2, __VA_ARGS__)

#define CHECK_LE(val1, val2, ...) \
    ABEL_INTERNAL_LOGGING_CHECK_OP(abel::default_logger_raw(), LE_, <=, val1, val2, __VA_ARGS__)

#define CHECK_LT(val1, val2, ...) \
    ABEL_INTERNAL_LOGGING_CHECK_OP(abel::default_logger_raw(), LT_, <, val1, val2, __VA_ARGS__)

#define CHECK_GE(val1, val2, ...) \
    ABEL_INTERNAL_LOGGING_CHECK_OP(abel::default_logger_raw(), GE_, >=, val1, val2, __VA_ARGS__)

#define CHECK_GT(val1, val2, ...) \
    ABEL_INTERNAL_LOGGING_CHECK_OP(abel::default_logger_raw(), EQ_, >, val1, val2, __VA_ARGS__)

#define CHECK_NEAR(val1, val2, margin, ...) \
    do {                                          \
            ABEL_CHECK_LE((val1), (val2) + (margin), __VA_ARGS__); \
            ABEL_CHECK_GE((val1), (val2) - (margin), __VA_ARGS__); \
    }while(0)

#if !defined(NDEBUG)

#define DCHECK(expr, ...) CHECK(expr, __VA_ARGS__)
#define DCHECK_EQ(expr1, expr2, ...) \
    CHECK_EQ(expr1, expr2, __VA_ARGS__)
#define DCHECK_NE(expr1, expr2, ...) \
    CHECK_NE(expr1, expr2, __VA_ARGS__)
#define DCHECK_LE(expr1, expr2, ...) \
    CHECK_LE(expr1, expr2, __VA_ARGS__)
#define DCHECK_GE(expr1, expr2, ...) \
    CHECK_GE(expr1, expr2, __VA_ARGS__)
#define DCHECK_LT(expr1, expr2, ...) \
    CHECK_LT(expr1, expr2, __VA_ARGS__)
#define DCHECK_GT(expr1, expr2, ...) \
    CHECK_GT(expr1, expr2, __VA_ARGS__)
#define DCHECK_NEAR(expr1, expr2, margin, ...) \
    CHECK_NEAR(expr1, expr2, __VA_ARGS__)
#else

#define ABEL_DCHECK(expr, ...) \
    while (0) ABEL_CHECK(expr, __VA_ARGS__)
#define ABEL_DCHECK_EQ(expr1, expr2, ...) \
    while (0) ABEL_CHECK_EQ(expr1, expr2, __VA_ARGS__)
#define ABEL_DCHECK_NE(expr1, expr2, ...) \
    while (0) ABEL_CHECK_NE(expr1, expr2, __VA_ARGS__)
#define ABEL_DCHECK_LE(expr1, expr2, ...) \
    while (0) ABEL_CHECK_LE(expr1, expr2, __VA_ARGS__)
#define ABEL_DCHECK_GE(expr1, expr2, ...) \
    while (0) ABEL_CHECK_GE(expr1, expr2, __VA_ARGS__)
#define ABEL_DCHECK_LT(expr1, expr2, ...) \
    while (0) ABEL_CHECK_LT(expr1, expr2, __VA_ARGS__)
#define ABEL_DCHECK_GT(expr1, expr2, ...) \
    while (0) ABEL_CHECK_GT(expr1, expr2, __VA_ARGS__)
#define ABEL_DCHECK_NEAR(expr1, expr2, margin, ...) \
    while (0) ABEL_CHECK_NEAR(expr1, expr2, __VA_ARGS__)
#endif


#endif  // ABEL_LOG_LOGGING_H_
