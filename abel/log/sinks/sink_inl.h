// Copyright(c) 2015-present, Gabi Melman & spdlog contributors.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#pragma once


#include "abel/log/common.h"

ABEL_FORCE_INLINE bool abel::sinks::sink::should_log(abel::level::level_enum msg_level) const {
    return msg_level >= level_.load(std::memory_order_relaxed);
}

ABEL_FORCE_INLINE void abel::sinks::sink::set_level(level::level_enum log_level) {
    level_.store(log_level, std::memory_order_relaxed);
}

ABEL_FORCE_INLINE abel::level::level_enum abel::sinks::sink::level() const {
    return static_cast<abel::level::level_enum>(level_.load(std::memory_order_relaxed));
}
