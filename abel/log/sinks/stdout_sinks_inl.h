// Copyright(c) 2015-present, Gabi Melman & spdlog contributors.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#pragma once

#include <memory>
#include "abel/log/details/console_globals.h"
#include "abel/log/pattern_formatter.h"


namespace abel {

namespace sinks {

template<typename ConsoleMutex>
ABEL_FORCE_INLINE stdout_sink_base<ConsoleMutex>::stdout_sink_base(FILE *file)
        : mutex_(ConsoleMutex::mutex()), file_(file),
          formatter_(abel::make_unique<abel::pattern_formatter>()) {}

template<typename ConsoleMutex>
ABEL_FORCE_INLINE void stdout_sink_base<ConsoleMutex>::log(const details::log_msg &msg) {
    std::lock_guard<mutex_t> lock(mutex_);
    memory_buf_t formatted;
    formatter_->format(msg, formatted);
    fwrite(formatted.data(), sizeof(char), formatted.size(), file_);
    fflush(file_); // flush every line to terminal
}

template<typename ConsoleMutex>
ABEL_FORCE_INLINE void stdout_sink_base<ConsoleMutex>::flush() {
    std::lock_guard<mutex_t> lock(mutex_);
    fflush(file_);
}

template<typename ConsoleMutex>
ABEL_FORCE_INLINE void stdout_sink_base<ConsoleMutex>::set_pattern(const std::string &pattern) {
    std::lock_guard<mutex_t> lock(mutex_);
    formatter_ = std::unique_ptr<abel::log_formatter>(new pattern_formatter(pattern));
}

template<typename ConsoleMutex>
ABEL_FORCE_INLINE void
stdout_sink_base<ConsoleMutex>::set_formatter(std::unique_ptr<abel::log_formatter> sink_formatter) {
    std::lock_guard<mutex_t> lock(mutex_);
    formatter_ = std::move(sink_formatter);
}

// stdout sink
template<typename ConsoleMutex>
ABEL_FORCE_INLINE stdout_sink<ConsoleMutex>::stdout_sink()
        : stdout_sink_base<ConsoleMutex>(stdout) {}

// stderr sink
template<typename ConsoleMutex>
ABEL_FORCE_INLINE stderr_sink<ConsoleMutex>::stderr_sink()
        : stdout_sink_base<ConsoleMutex>(stderr) {}

} // namespace sinks

// factory methods
template<typename Factory>
ABEL_FORCE_INLINE std::shared_ptr<logger> stdout_logger_mt(const std::string &logger_name) {
    return Factory::template create<sinks::stdout_sink_mt>(logger_name);
}

template<typename Factory>
ABEL_FORCE_INLINE std::shared_ptr<logger> stdout_logger_st(const std::string &logger_name) {
    return Factory::template create<sinks::stdout_sink_st>(logger_name);
}

template<typename Factory>
ABEL_FORCE_INLINE std::shared_ptr<logger> stderr_logger_mt(const std::string &logger_name) {
    return Factory::template create<sinks::stderr_sink_mt>(logger_name);
}

template<typename Factory>
ABEL_FORCE_INLINE std::shared_ptr<logger> stderr_logger_st(const std::string &logger_name) {
    return Factory::template create<sinks::stderr_sink_st>(logger_name);
}
}  // namespace abel
