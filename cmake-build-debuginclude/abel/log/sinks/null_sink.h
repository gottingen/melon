//
// Copyright(c) 2015 Gabi Melman.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#pragma once

#include <abel/log/details/null_mutex.h>
#include <abel/log/sinks/base_sink.h>

#include <mutex>

namespace abel {
namespace log {
namespace sinks {

template<typename Mutex>
class null_sink : public base_sink<Mutex> {
protected:
    void sink_it_ (const details::log_msg &) override { }
    void flush_ () override { }
};

using null_sink_mt = null_sink<std::mutex>;
using null_sink_st = null_sink<details::null_mutex>;

} // namespace sinks
} //namespace log
} // namespace abel
