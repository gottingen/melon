// Copyright(c) 2015-present, Gabi Melman & spdlog contributors.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#pragma once

#include <memory>
#include "abel/log/common.h"
#include "abel/log/pattern_formatter.h"


template<typename Mutex>
ABEL_FORCE_INLINE abel::sinks::base_sink<Mutex>::base_sink()
        : formatter_{abel::make_unique<abel::pattern_formatter>()} {}

template<typename Mutex>
ABEL_FORCE_INLINE abel::sinks::base_sink<Mutex>::base_sink(std::unique_ptr<abel::log_formatter> formatter)
        : formatter_{std::move(formatter)} {}

template<typename Mutex>
void ABEL_FORCE_INLINE abel::sinks::base_sink<Mutex>::log(const details::log_msg &msg) {
    std::lock_guard<Mutex> lock(mutex_);
    sink_it_(msg);
}

template<typename Mutex>
void ABEL_FORCE_INLINE abel::sinks::base_sink<Mutex>::flush() {
    std::lock_guard<Mutex> lock(mutex_);
    flush_();
}

template<typename Mutex>
void ABEL_FORCE_INLINE abel::sinks::base_sink<Mutex>::set_pattern(const std::string &pattern) {
    std::lock_guard<Mutex> lock(mutex_);
    set_pattern_(pattern);
}

template<typename Mutex>
void ABEL_FORCE_INLINE
abel::sinks::base_sink<Mutex>::set_formatter(std::unique_ptr<abel::log_formatter> sink_formatter) {
    std::lock_guard<Mutex> lock(mutex_);
    set_formatter_(std::move(sink_formatter));
}

template<typename Mutex>
void ABEL_FORCE_INLINE abel::sinks::base_sink<Mutex>::set_pattern_(const std::string &pattern) {
    set_formatter_(abel::make_unique<abel::pattern_formatter>(pattern));
}

template<typename Mutex>
void ABEL_FORCE_INLINE
abel::sinks::base_sink<Mutex>::set_formatter_(std::unique_ptr<abel::log_formatter> sink_formatter) {
    formatter_ = std::move(sink_formatter);
}
