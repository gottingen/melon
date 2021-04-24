//
// Created by liyinbin on 2020/7/31.
//

#ifndef ABEL_LOG_LOGGING_H_
#define ABEL_LOG_LOGGING_H_

#include "abel/log/log.h"
#include "abel/utility/every.h"
#include "abel/chrono/clock.h"

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
#define LOG_IF_TRACE(logger, condition, ...) LOG_CALL_IF(logger, abel::level::trace, condition, __VA_ARGS__)
#define LOG_EVERY_N_TRACE(logger, N, ...)  LOG_CALL_IF_EVERY_N(logger, abel::level::trace, true, N, __VA_ARGS__)
#define LOG_IF_EVERY_N_TRACE(logger, condition, N, ...)  LOG_CALL_IF_EVERY_N(logger, abel::level::trace, condition, N, __VA_ARGS__)
#define LOG_FIRST_N_TRACE(logger, N, ...) LOG_CALL_IF_FIRST_N(logger, abel::level::trace, true, __VA_ARGS__)
#define LOG_IF_FIRST_N_TRACE(logger, condition, N, ...) LOG_CALL_IF_FIRST_N(logger, abel::level::trace, true, __VA_ARGS__)
#define LOG_EVERY_SECOND_TRACE(logger, ...) LOG_CALL_IF_EVERY_SECOND(logger, abel::level::trace, true, __VA_ARGS__)
#define LOG_IF_EVERY_SECOND_TRACE(logger, condition, ...) LOG_CALL_IF_EVERY_SECOND(logger, abel::level::trace, condition, __VA_ARGS__)
// default log
#define DLOG_TRACE(...) LOG_TRACE(abel::default_logger_raw(), __VA_ARGS__)
#define DLOG_IF_TRACE(condition, ...) LOG_IF_TRACE(abel::default_logger_raw(), condition, __VA_ARGS__)
#define DLOG_EVERY_N_TRACE(N, ...) LOG_EVERY_N_TRACE(abel::default_logger_raw(), N, __VA_ARGS__)
#define DLOG_IF_EVERY_N_TRACE(condition, N, ...) LOG_IF_EVERY_N_TRACE(abel::default_logger_raw(), condition, N, __VA_ARGS__)
#define DLOG_FIRST_N_TRACE(N, ...) LOG_FIRST_N_TRACE(abel::default_logger_raw(), N, __VA_ARGS__)
#define DLOG_IF_FIRST_N_TRACE(condition, N, ...) LOG_IF_FIRST_N_TRACE(abel::default_logger_raw(), condition, N, __VA_ARGS__)
#define DLOG_EVERY_SECOND_TRACE(...) LOG_EVERY_SECOND_TRACE(abel::default_logger_raw(), __VA_ARGS__)
#define DLOG_IF_EVERY_SECOND_TRACE(condition, ...) LOG_IF_EVERY_SECOND_TRACE(abel::default_logger_raw(),condition, __VA_ARGS__)

#else
#define LOG_TRACE(logger, ...) (void)0
#define LOG_IF_TRACE(logger, condition, ...) (void)0
#define LOG_EVERY_N_TRACE(logger, N, ...)  (void)0
#define LOG_IF_EVERY_N_TRACE(logger, condition, N, ...)  (void)0
#define LOG_FIRST_N_TRACE(logger, N, ...) (void)0
#define LOG_IF_FIRST_N_TRACE(logger, condition, N, ...) (void)0
#define LOG_EVERY_SECOND_TRACE(logger, ...) (void)0
#define LOG_IF_EVERY_SECOND_TRACE(logger, condition, ...) (void)0
// default log
#define DLOG_TRACE(...) (void)0
#define DLOG_IF_TRACE(condition, ...) (void)0
#define DLOG_EVERY_N_TRACE(N, ...) (void)0
#define DLOG_IF_EVERY_N_TRACE(condition, N, ...) (void)0
#define DLOG_FIRST_N_TRACE(N, ...) (void)0
#define DLOG_IF_FIRST_N_TRACE(condition, N, ...)(void)0
#define DLOG_EVERY_SECOND_TRACE(...) (void)0
#define DLOG_IF_EVERY_SECOND_TRACE(condition, ...) (void)0

#endif

#if LOG_ACTIVE_LEVEL <= LOG_LEVEL_DEBUG
#define LOG_DEBUG(logger, ...) LOG_CALL(logger, abel::level::debug, __VA_ARGS__)
#define LOG_IF_DEBUG(logger, condition, ...) LOG_CALL_IF(logger, abel::level::debug, condition, __VA_ARGS__)
#define LOG_EVERY_N_DEBUG(logger, N, ...)  LOG_CALL_IF_EVERY_N(logger, abel::level::debug, true, N, __VA_ARGS__)
#define LOG_IF_EVERY_N_DEBUG(logger, condition, N, ...)  LOG_CALL_IF_EVERY_N(logger, abel::level::debug, condition, N, __VA_ARGS__)
#define LOG_FIRST_N_DEBUG(logger, N, ...) LOG_CALL_IF_FIRST_N(logger, abel::level::debug, true, __VA_ARGS__)
#define LOG_IF_FIRST_N_DEBUG(logger, condition, N, ...) LOG_CALL_IF_FIRST_N(logger, abel::level::debug, true, __VA_ARGS__)
#define LOG_EVERY_SECOND_DEBUG(logger, ...) LOG_CALL_IF_EVERY_SECOND(logger, abel::level::debug, true, __VA_ARGS__)
#define LOG_IF_EVERY_SECOND_DEBUG(logger, condition, ...) LOG_CALL_IF_EVERY_SECOND(logger, abel::level::debug, condition, __VA_ARGS__)
// default log
#define DLOG_DEBUG(...) LOG_DEBUG(abel::default_logger_raw(), __VA_ARGS__)
#define DLOG_IF_DEBUG(condition, ...) LOG_IF_DEBUG(abel::default_logger_raw(), condition, __VA_ARGS__)
#define DLOG_EVERY_N_DEBUG(N, ...) LOG_EVERY_N_DEBUG(abel::default_logger_raw(), N, __VA_ARGS__)
#define DLOG_IF_EVERY_N_DEBUG(condition, N, ...) LOG_IF_EVERY_N_DEBUG(abel::default_logger_raw(), condition, N, __VA_ARGS__)
#define DLOG_FIRST_N_DEBUG(N, ...) LOG_FIRST_N_DEBUG(abel::default_logger_raw(), N, __VA_ARGS__)
#define DLOG_IF_FIRST_N_DEBUG(condition, N, ...) LOG_IF_FIRST_N_TRACE(abel::default_logger_raw(), condition, N, __VA_ARGS__)
#define DLOG_EVERY_SECOND_DEBUG(...) LOG_EVERY_SECOND_DEBUG(abel::default_logger_raw(), __VA_ARGS__)
#define DLOG_IF_EVERY_SECOND_DEBUG(condition, ...) LOG_IF_EVERY_SECOND_DEBUG(abel::default_logger_raw(),condition, __VA_ARGS__)

#else
#define LOG_DEBUG(logger, ...) (void)0
#define LOG_IF_DEBUG(logger, condition, ...) (void)0
#define LOG_EVERY_N_DEBUG(logger, N, ...)  (void)0
#define LOG_IF_EVERY_N_DEBUG(logger, condition, N, ...)  (void)0
#define LOG_FIRST_N_DEBUG(logger, N, ...) (void)0
#define LOG_IF_FIRST_N_DEBUG(logger, condition, N, ...) (void)0
#define LOG_EVERY_SECOND_DEBUG(logger, ...)(void)0
#define LOG_IF_EVERY_SECOND_DEBUG(logger, condition, ...)(void)0
// default log
#define DLOG_DEBUG(...) (void)0
#define DLOG_IF_DEBUG(condition, ...) (void)0
#define DLOG_EVERY_N_DEBUG(N, ...) (void)0
#define DLOG_IF_EVERY_N_DEBUG(condition, N, ...) (void)0
#define DLOG_FIRST_N_DEBUG(N, ...) (void)0
#define DLOG_IF_FIRST_N_DEBUG(condition, N, ...) (void)0
#define DLOG_EVERY_SECOND_DEBUG(...) (void)0
#define DLOG_IF_EVERY_SECOND_DEBUG(condition, ...) (void)0
#endif

#if LOG_ACTIVE_LEVEL <= LOG_LEVEL_INFO
#define LOG_INFO(logger, ...) LOG_CALL(logger, abel::level::info, __VA_ARGS__)
#define LOG_IF_INFO(logger, condition, ...) LOG_CALL_IF(logger, abel::level::info, condition, __VA_ARGS__)
#define LOG_EVERY_N_INFO(logger, N, ...)  LOG_CALL_IF_EVERY_N(logger, abel::level::info, true, N, __VA_ARGS__)
#define LOG_IF_EVERY_N_INFO(logger, condition, N, ...)  LOG_CALL_IF_EVERY_N(logger, abel::level::info, condition, N, __VA_ARGS__)
#define LOG_FIRST_N_INFO(logger, N, ...) LOG_CALL_IF_FIRST_N(logger, abel::level::info, true, __VA_ARGS__)
#define LOG_IF_FIRST_N_INFO(logger, condition, N, ...) LOG_CALL_IF_FIRST_N(logger, abel::level::info, true, __VA_ARGS__)
#define LOG_EVERY_SECOND_INFO(logger, ...) LOG_CALL_IF_EVERY_SECOND(logger, abel::level::info, true, __VA_ARGS__)
#define LOG_IF_EVERY_SECOND_INFO(logger, condition, ...) LOG_CALL_IF_EVERY_SECOND(logger, abel::level::info, condition, __VA_ARGS__)
// default log
#define DLOG_INFO(...) LOG_INFO(abel::default_logger_raw(), __VA_ARGS__)
#define DLOG_IF_INFO(condition, ...) LOG_IF_INFO(abel::default_logger_raw(), condition, __VA_ARGS__)
#define DLOG_EVERY_N_INFO(N, ...) LOG_EVERY_N_INFO(abel::default_logger_raw(), N, __VA_ARGS__)
#define DLOG_IF_EVERY_N_INFO(condition, N, ...) LOG_IF_EVERY_N_INFO(abel::default_logger_raw(), condition, N, __VA_ARGS__)
#define DLOG_FIRST_N_INFO(N, ...) LOG_FIRST_N_INFO(abel::default_logger_raw(), N, __VA_ARGS__)
#define DLOG_IF_FIRST_N_INFO(condition, N, ...) LOG_IF_FIRST_N_INFO(abel::default_logger_raw(), condition, N, __VA_ARGS__)
#define DLOG_EVERY_SECOND_INFO(...) LOG_EVERY_SECOND_INFO(abel::default_logger_raw(), __VA_ARGS__)
#define DLOG_IF_EVERY_SECOND_INFO(condition, ...) LOG_IF_EVERY_SECOND_INFO(abel::default_logger_raw(),condition, __VA_ARGS__)

#else
#define LOG_INFO(logger, ...) (void)0
#define LOG_IF_INFO(logger, condition, ...) (void)0
#define LOG_EVERY_N_INFO(logger, N, ...) (void)0
#define LOG_IF_EVERY_N_INFO(logger, condition, N, ...)  (void)0
#define LOG_FIRST_N_INFO(logger, N, ...) (void)0
#define LOG_IF_FIRST_N_INFO(logger, condition, N, ...) (void)0
#define LOG_EVERY_SECOND_INFO(logger, ...) (void)0
#define LOG_IF_EVERY_SECOND_INFO(logger, condition, ...) (void)0
// default log
#define DLOG_INFO(...) (void)0
#define DLOG_IF_INFO(condition, ...) (void)0
#define DLOG_EVERY_N_INFO(N, ...) (void)0
#define DLOG_IF_EVERY_N_INFO(condition, N, ...) (void)0
#define DLOG_FIRST_N_INFO(N, ...) (void)0
#define DLOG_IF_FIRST_N_INFO(condition, N, ...) (void)0
#define DLOG_EVERY_SECOND_INFO(...) (void)0
#define DLOG_IF_EVERY_SECOND_INFO(condition, ...) (void)0
#endif

#if LOG_ACTIVE_LEVEL <= LOG_LEVEL_WARN
#define LOG_WARN(logger, ...) LOG_CALL(logger, abel::level::warn, __VA_ARGS__)
#define LOG_IF_WARN(logger, condition, ...) LOG_CALL_IF(logger, abel::level::warn, condition, __VA_ARGS__)
#define LOG_EVERY_N_WARN(logger, N, ...)  LOG_CALL_IF_EVERY_N(logger, abel::level::warn, true, N, __VA_ARGS__)
#define LOG_IF_EVERY_N_WARN(logger, condition, N, ...)  LOG_CALL_IF_EVERY_N(logger, abel::level::warn, condition, N, __VA_ARGS__)
#define LOG_FIRST_N_WARN(logger, N, ...) LOG_CALL_IF_FIRST_N(logger, abel::level::warn, true, __VA_ARGS__)
#define LOG_IF_FIRST_N_WARN(logger, condition, N, ...) LOG_CALL_IF_FIRST_N(logger, abel::level::warn, true, __VA_ARGS__)
#define LOG_EVERY_SECOND_WARN(logger, ...) LOG_CALL_IF_EVERY_SECOND(logger, abel::level::warn, true, __VA_ARGS__)
#define LOG_IF_EVERY_SECOND_WARN(logger, condition, ...) LOG_CALL_IF_EVERY_SECOND(logger, abel::level::warn, condition, __VA_ARGS__)
// default log
#define DLOG_WARN(...) LOG_WARN(abel::default_logger_raw(), __VA_ARGS__)
#define DLOG_IF_WARN(condition, ...) LOG_IF_WARN(abel::default_logger_raw(), condition, __VA_ARGS__)
#define DLOG_EVERY_N_WARN(N, ...) LOG_EVERY_N_WARN(abel::default_logger_raw(), N, __VA_ARGS__)
#define DLOG_IF_EVERY_N_WARN(condition, N, ...) LOG_IF_EVERY_N_WARN(abel::default_logger_raw(), condition, N, __VA_ARGS__)
#define DLOG_FIRST_N_WARN(N, ...) LOG_FIRST_N_WARN(abel::default_logger_raw(), N, __VA_ARGS__)
#define DLOG_IF_FIRST_N_WARN(condition, N, ...) LOG_IF_FIRST_N_WARN(abel::default_logger_raw(), condition, N, __VA_ARGS__)
#define DLOG_EVERY_SECOND_WARN(...) LOG_EVERY_SECOND_WARN(abel::default_logger_raw(), __VA_ARGS__)
#define DLOG_IF_EVERY_SECOND_WARN(condition, ...) LOG_IF_EVERY_SECOND_WARN(abel::default_logger_raw(),condition, __VA_ARGS__)

#else
#define LOG_WARN(logger, ...) (void)0
#define LOG_IF_WARN(logger, condition, ...) (void)0
#define LOG_EVERY_N_WARN(logger, N, ...)  (void)0
#define LOG_IF_EVERY_N_WARN(logger, condition, N, ...)  (void)0
#define LOG_FIRST_N_WARN(logger, N, ...) (void)0
#define LOG_IF_FIRST_N_WARN(logger, condition, N, ...) (void)0
#define LOG_EVERY_SECOND_WARN(logger, ...) (void)0
#define LOG_IF_EVERY_SECOND_WARN(logger, condition, ...) (void)0
// default log
#define DLOG_WARN(...) (void)0
#define DLOG_IF_WARN(condition, ...) (void)0
#define DLOG_EVERY_N_WARN(N, ...) (void)0
#define DLOG_IF_EVERY_N_WARN(condition, N, ...) (void)0
#define DLOG_FIRST_N_WARN(N, ...) (void)0
#define DLOG_IF_FIRST_N_WARN(condition, N, ...) (void)0
#define DLOG_EVERY_SECOND_WARN(...) (void)0
#define DLOG_IF_EVERY_SECOND_WARN(condition, ...) (void)0
#endif

#if LOG_ACTIVE_LEVEL <= LOG_LEVEL_ERROR

#define LOG_ERROR(logger, ...) LOG_CALL(logger, abel::level::err, __VA_ARGS__)
#define LOG_IF_ERROR(logger, condition, ...) LOG_CALL_IF(logger, abel::level::err, condition, __VA_ARGS__)
#define LOG_EVERY_N_ERROR(logger, N, ...)  LOG_CALL_IF_EVERY_N(logger, abel::level::err, true, N, __VA_ARGS__)
#define LOG_IF_EVERY_N_ERROR(logger, condition, N, ...)  LOG_CALL_IF_EVERY_N(logger, abel::level::err, condition, N, __VA_ARGS__)
#define LOG_FIRST_N_ERROR(logger, N, ...) LOG_CALL_IF_FIRST_N(logger, abel::level::err, true, __VA_ARGS__)
#define LOG_IF_FIRST_N_ERROR(logger, condition, N, ...) LOG_CALL_IF_FIRST_N(logger, abel::level::err, true, __VA_ARGS__)
#define LOG_EVERY_SECOND_ERROR(logger, ...) LOG_CALL_IF_EVERY_SECOND(logger, abel::level::err, true, __VA_ARGS__)
#define LOG_IF_EVERY_SECOND_ERROR(logger, condition, ...) LOG_CALL_IF_EVERY_SECOND(logger, abel::level::err, condition, __VA_ARGS__)
// default log
#define DLOG_ERROR(...) LOG_ERROR(abel::default_logger_raw(), __VA_ARGS__)
#define DLOG_IF_ERROR(condition, ...) LOG_IF_ERROR(abel::default_logger_raw(), condition, __VA_ARGS__)
#define DLOG_EVERY_N_ERROR(N, ...) LOG_EVERY_N_ERROR(abel::default_logger_raw(), N, __VA_ARGS__)
#define DLOG_IF_EVERY_N_ERROR(condition, N, ...) LOG_IF_EVERY_N_ERROR(abel::default_logger_raw(), condition, N, __VA_ARGS__)
#define DLOG_FIRST_N_ERROR(N, ...) LOG_FIRST_N_ERROR(abel::default_logger_raw(), N, __VA_ARGS__)
#define DLOG_IF_FIRST_N_ERROR(condition, N, ...) LOG_IF_FIRST_N_ERROR(abel::default_logger_raw(), condition, N, __VA_ARGS__)
#define DLOG_EVERY_SECOND_ERROR(...) LOG_EVERY_SECOND_ERROR(abel::default_logger_raw(), __VA_ARGS__)
#define DLOG_IF_EVERY_SECOND_ERROR(condition, ...) LOG_IF_EVERY_SECOND_ERROR(abel::default_logger_raw(),condition, __VA_ARGS__)

#else
#define LOG_ERROR(logger, ...) (void)0
#define LOG_IF_ERROR(logger, condition, ...) (void)0
#define LOG_EVERY_N_ERROR(logger, N, ...)  (void)0
#define LOG_IF_EVERY_N_ERROR(logger, condition, N, ...)  (void)0
#define LOG_FIRST_N_ERROR(logger, N, ...) (void)0
#define LOG_IF_FIRST_N_ERROR(logger, condition, N, ...) (void)0
#define LOG_EVERY_SECOND_ERROR(logger, ...) (void)0
#define LOG_IF_EVERY_SECOND_ERROR(logger, condition, ...) (void)0
// default log
#define DLOG_ERROR(...) (void)0
#define DLOG_IF_ERROR(condition, ...) (void)0
#define DLOG_EVERY_N_ERROR(N, ...) (void)0
#define DLOG_IF_EVERY_N_ERROR(condition, N, ...) (void)0
#define DLOG_FIRST_N_ERROR(N, ...) (void)0
#define DLOG_IF_FIRST_N_ERROR(condition, N, ...) (void)0
#define DLOG_EVERY_SECOND_ERROR(...) (void)0
#define DLOG_IF_EVERY_SECOND_ERROR(condition, ...) (void)0

#endif

#if LOG_ACTIVE_LEVEL <= LOG_LEVEL_CRITICAL
#define LOG_CRITICAL(logger, ...) LOG_CALL(logger, abel::level::critical, __VA_ARGS__)
#define LOG_IF_CRITICAL(logger, condition, ...) LOG_CALL_IF(logger, abel::level::critical, condition, __VA_ARGS__)
#define LOG_EVERY_N_CRITICAL(logger, N, ...)  LOG_CALL_IF_EVERY_N(logger, abel::level::critical, true, N, __VA_ARGS__)
#define LOG_IF_EVERY_N_CRITICAL(logger, condition, N, ...)  LOG_CALL_IF_EVERY_N(logger, abel::level::critical, condition, N, __VA_ARGS__)
#define LOG_FIRST_N_CRITICAL(logger, N, ...) LOG_CALL_IF_FIRST_N(logger, abel::level::critical, true, __VA_ARGS__)
#define LOG_IF_FIRST_N_CRITICAL(logger, condition, N, ...) LOG_CALL_IF_FIRST_N(logger, abel::level::critical, true, __VA_ARGS__)
#define LOG_EVERY_SECOND_CRITICAL(logger, ...) LOG_CALL_IF_EVERY_SECOND(logger, abel::level::critical, true, __VA_ARGS__)
#define LOG_IF_EVERY_SECOND_CRITICAL(logger, condition, ...) LOG_CALL_IF_EVERY_SECOND(logger, abel::level::critical, condition, __VA_ARGS__)
// default log
#define DLOG_CRITICAL(...) LOG_CRITICAL(abel::default_logger_raw(), __VA_ARGS__)
#define DLOG_IF_CRITICAL(condition, ...) LOG_IF_CRITICAL(abel::default_logger_raw(), condition, __VA_ARGS__)
#define DLOG_EVERY_N_CRITICAL(N, ...) LOG_EVERY_N_CRITICAL(abel::default_logger_raw(), N, __VA_ARGS__)
#define DLOG_IF_EVERY_N_CRITICAL(condition, N, ...) LOG_IF_EVERY_N_CRITICAL(abel::default_logger_raw(), condition, N, __VA_ARGS__)
#define DLOG_FIRST_N_CRITICAL(N, ...) LOG_FIRST_N_CRITICAL(abel::default_logger_raw(), N, __VA_ARGS__)
#define DLOG_IF_FIRST_N_CRITICAL(condition, N, ...) LOG_IF_FIRST_N_CRITICAL(abel::default_logger_raw(), condition, N, __VA_ARGS__)
#define DLOG_EVERY_SECOND_CRITICAL(...) LOG_EVERY_SECOND_CRITICAL(abel::default_logger_raw(), __VA_ARGS__)
#define DLOG_IF_EVERY_SECOND_CRITICAL(condition, ...) LOG_IF_EVERY_SECOND_CRITICAL(abel::default_logger_raw(),condition, __VA_ARGS__)

#else
#define LOG_CRITICAL(logger, ...) (void)0
#define LOG_IF_CRITICAL(logger, condition, ...) (void)0
#define LOG_EVERY_N_CRITICAL(logger, N, ...)  (void)0
#define LOG_IF_EVERY_N_CRITICAL(logger, condition, N, ...)  (void)0
#define LOG_FIRST_N_CRITICAL(logger, N, ...) (void)0
#define LOG_IF_FIRST_N_CRITICAL(logger, condition, N, ...) (void)0
#define LOG_EVERY_SECOND_CRITICAL(logger, ...) (void)0
#define LOG_IF_EVERY_SECOND_CRITICAL(logger, condition, ...) (void)0
// default log
#define DLOG_CRITICAL(...) (void)0
#define DLOG_IF_CRITICAL(condition, ...) (void)0
#define DLOG_EVERY_N_CRITICAL(N, ...) (void)0
#define DLOG_IF_EVERY_N_CRITICAL(condition, N, ...) (void)0
#define DLOG_FIRST_N_CRITICAL(N, ...) (void)0
#define DLOG_IF_FIRST_N_CRITICAL(condition, N, ...) (void)0
#define DLOG_EVERY_SECOND_CRITICAL(...) (void)0
#define DLOG_IF_EVERY_SECOND_CRITICAL(condition, ...) (void)0

#endif


#define CHECK(logger, condition, ...) \
    do {                       \
        if (ABEL_UNLIKELY(!(condition))) { \
            LOG_CRITICAL(logger, __VA_ARGS__); \
            ::abort();                          \
        }                                       \
    } while(0)

#define DCHECK_MSG(condition, ...) \
    do {                        \
        if (ABEL_UNLIKELY(!(condition))) {       \
            DLOG_CRITICAL(__VA_ARGS__);          \
            ::abort();                           \
        }                                        \
    } while(0)

#define DCHECK(condition) assert(condition)
#define DCHECK_EQ(val1, val2) assert((val1) == (val2))
#define DCHECK_NE(val1, val2) assert((val1) != (val2))
#define DCHECK_LE(val1, val2) assert((val1) <= (val2))
#define DCHECK_LT(val1, val2) assert((val1) < (val2))
#define DCHECK_GE(val1, val2) assert((val1) >= (val2))
#define DCHECK_GT(val1, val2) assert((val1) > (val2))

#endif  // ABEL_LOG_LOGGING_H_
