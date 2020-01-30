//
// Copyright(c) 2015 Gabi Melman.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#pragma once

#include <memory>
#include <string>

// create logger with given name, sinks and the default pattern formatter
// all other ctors will call this one
template<typename It>
inline abel_log::logger::logger (std::string logger_name, const It &begin, const It &end)
    : name_(std::move(logger_name)), sinks_(begin, end), level_(level::info), flush_level_(level::off),
      last_err_time_(0), msg_counter_(1) // message counter will start from 1. 0-message id will be
// reserved for controll messages
{
    err_handler_ = [this] (const std::string &msg) { this->default_err_handler_(msg); };
}

// ctor with sinks as init list
inline abel_log::logger::logger (std::string logger_name, sinks_init_list sinks_list)
    : logger(std::move(logger_name), sinks_list.begin(), sinks_list.end()) {
}

// ctor with single sink
inline abel_log::logger::logger (std::string logger_name, abel_log::sink_ptr single_sink)
    : logger(std::move(logger_name), {std::move(single_sink)}) {
}

inline abel_log::logger::~logger () = default;

inline void abel_log::logger::set_formatter (std::unique_ptr<abel_log::formatter> f) {
    for (auto &sink : sinks_) {
        sink->set_formatter(f->clone());
    }
}

inline void abel_log::logger::set_pattern (std::string pattern, pattern_time_type time_type) {
    set_formatter(std::unique_ptr<abel_log::formatter>(new pattern_formatter(std::move(pattern), time_type)));
}

template<typename... Args>
inline void abel_log::logger::log (level::level_enum lvl, const char *fmt, const Args &... args) {
    if (!should_log(lvl)) {
        return;
    }

    try {
        details::log_msg log_msg(&name_, lvl);
        fmt::format_to(log_msg.raw, fmt, args...);
        sink_it_(log_msg);
    }
    SPDLOG_CATCH_AND_HANDLE
}

template<typename... Args>
inline void abel_log::logger::log (level::level_enum lvl, const char *msg) {
    if (!should_log(lvl)) {
        return;
    }
    try {
        details::log_msg log_msg(&name_, lvl);
        fmt::format_to(log_msg.raw, "{}", msg);
        sink_it_(log_msg);
    }
    SPDLOG_CATCH_AND_HANDLE
}

template<typename T>
inline void abel_log::logger::log (level::level_enum lvl, const T &msg) {
    if (!should_log(lvl)) {
        return;
    }
    try {
        details::log_msg log_msg(&name_, lvl);
        fmt::format_to(log_msg.raw, "{}", msg);
        sink_it_(log_msg);
    }
    SPDLOG_CATCH_AND_HANDLE
}

template<typename... Args>
inline void abel_log::logger::trace (const char *fmt, const Args &... args) {
    log(level::trace, fmt, args...);
}

template<typename... Args>
inline void abel_log::logger::debug (const char *fmt, const Args &... args) {
    log(level::debug, fmt, args...);
}

template<typename... Args>
inline void abel_log::logger::info (const char *fmt, const Args &... args) {
    log(level::info, fmt, args...);
}

template<typename... Args>
inline void abel_log::logger::warn (const char *fmt, const Args &... args) {
    log(level::warn, fmt, args...);
}

template<typename... Args>
inline void abel_log::logger::error (const char *fmt, const Args &... args) {
    log(level::err, fmt, args...);
}

template<typename... Args>
inline void abel_log::logger::critical (const char *fmt, const Args &... args) {
    log(level::critical, fmt, args...);
}

template<typename T>
inline void abel_log::logger::trace (const T &msg) {
    log(level::trace, msg);
}

template<typename T>
inline void abel_log::logger::debug (const T &msg) {
    log(level::debug, msg);
}

template<typename T>
inline void abel_log::logger::info (const T &msg) {
    log(level::info, msg);
}

template<typename T>
inline void abel_log::logger::warn (const T &msg) {
    log(level::warn, msg);
}

template<typename T>
inline void abel_log::logger::error (const T &msg) {
    log(level::err, msg);
}

template<typename T>
inline void abel_log::logger::critical (const T &msg) {
    log(level::critical, msg);
}

#ifdef SPDLOG_WCHAR_TO_UTF8_SUPPORT
template<typename... Args>
inline void abel_log::logger::log(level::level_enum lvl, const wchar_t *fmt, const Args &... args)
{
    if (!should_log(lvl))
    {
        return;
    }

    decltype(wstring_converter_)::byte_string utf8_string;

    try
    {
        {
            std::lock_guard<std::mutex> lock(wstring_converter_mutex_);
            utf8_string = wstring_converter_.to_bytes(fmt);
        }
        log(lvl, utf8_string.c_str(), args...);
    }
    SPDLOG_CATCH_AND_HANDLE
}

template<typename... Args>
inline void abel_log::logger::trace(const wchar_t *fmt, const Args &... args)
{
    log(level::trace, fmt, args...);
}

template<typename... Args>
inline void abel_log::logger::debug(const wchar_t *fmt, const Args &... args)
{
    log(level::debug, fmt, args...);
}

template<typename... Args>
inline void abel_log::logger::info(const wchar_t *fmt, const Args &... args)
{
    log(level::info, fmt, args...);
}

template<typename... Args>
inline void abel_log::logger::warn(const wchar_t *fmt, const Args &... args)
{
    log(level::warn, fmt, args...);
}

template<typename... Args>
inline void abel_log::logger::error(const wchar_t *fmt, const Args &... args)
{
    log(level::err, fmt, args...);
}

template<typename... Args>
inline void abel_log::logger::critical(const wchar_t *fmt, const Args &... args)
{
    log(level::critical, fmt, args...);
}

#endif // SPDLOG_WCHAR_TO_UTF8_SUPPORT

//
// name and level
//
inline const std::string &abel_log::logger::name () const {
    return name_;
}

inline void abel_log::logger::set_level (abel_log::level::level_enum log_level) {
    level_.store(log_level);
}

inline void abel_log::logger::set_error_handler (abel_log::log_err_handler err_handler) {
    err_handler_ = std::move(err_handler);
}

inline abel_log::log_err_handler abel_log::logger::error_handler () {
    return err_handler_;
}

inline void abel_log::logger::flush () {
    try {
        flush_();
    }
    SPDLOG_CATCH_AND_HANDLE
}

inline void abel_log::logger::flush_on (level::level_enum log_level) {
    flush_level_.store(log_level);
}

inline bool abel_log::logger::should_flush_ (const details::log_msg &msg) {
    auto flush_level = flush_level_.load(std::memory_order_relaxed);
    return (msg.level >= flush_level) && (msg.level != level::off);
}

inline abel_log::level::level_enum abel_log::logger::level () const {
    return static_cast<abel_log::level::level_enum>(level_.load(std::memory_order_relaxed));
}

inline bool abel_log::logger::should_log (abel_log::level::level_enum msg_level) const {
    return msg_level >= level_.load(std::memory_order_relaxed);
}

//
// protected virtual called at end of each user log call (if enabled) by the
// line_logger
//
inline void abel_log::logger::sink_it_ (details::log_msg &msg) {
#if defined(SPDLOG_ENABLE_MESSAGE_COUNTER)
    incr_msg_counter_(msg);
#endif
    for (auto &sink : sinks_) {
        if (sink->should_log(msg.level)) {
            sink->log(msg);
        }
    }

    if (should_flush_(msg)) {
        flush();
    }
}

inline void abel_log::logger::flush_ () {
    for (auto &sink : sinks_) {
        sink->flush();
    }
}

inline void abel_log::logger::default_err_handler_ (const std::string &msg) {
    auto now = abel::now();
    auto secs = abel::to_unix_seconds(now);
    if (secs - last_err_time_ < 60) {
        return;
    }
    last_err_time_ = secs;
    auto tm_time = abel::local_tm(now);
    char date_buf[100];
    std::strftime(date_buf, sizeof(date_buf), "%Y-%m-%d %H:%M:%S", &tm_time);
    fmt::print(stderr, "[*** LOG ERROR ***] [{}] [{}] {}\n", date_buf, name(), msg);
}

inline void abel_log::logger::incr_msg_counter_ (details::log_msg &msg) {
    msg.msg_id = msg_counter_.fetch_add(1, std::memory_order_relaxed);
}

inline const std::vector<abel_log::sink_ptr> &abel_log::logger::sinks () const {
    return sinks_;
}

inline std::vector<abel_log::sink_ptr> &abel_log::logger::sinks () {
    return sinks_;
}
