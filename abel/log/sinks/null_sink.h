// Copyright(c) 2015-present, Gabi Melman & spdlog contributors.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#pragma once

#include <mutex>
#include "abel/log/details/null_mutex.h"
#include "abel/log/sinks/base_sink.h"
#include "abel/log/details/synchronous_factory.h"


namespace abel {
namespace sinks {

template<typename Mutex>
class null_sink : public base_sink<Mutex> {
  protected:
    void sink_it_(const details::log_msg &) override {}

    void flush_() override {}
};

using null_sink_mt = null_sink<details::null_mutex>;
using null_sink_st = null_sink<details::null_mutex>;

} // namespace sinks

template<typename Factory = abel::synchronous_factory>
inline std::shared_ptr<logger> null_logger_mt(const std::string &logger_name) {
    auto null_logger = Factory::template create<sinks::null_sink_mt>(logger_name);
    null_logger->set_level(level::off);
    return null_logger;
}

template<typename Factory = abel::synchronous_factory>
inline std::shared_ptr<logger> null_logger_st(const std::string &logger_name) {
    auto null_logger = Factory::template create<sinks::null_sink_st>(logger_name);
    null_logger->set_level(level::off);
    return null_logger;
}

}  // namespace abel
