// Copyright(c) 2015-present, Gabi Melman & spdlog contributors.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#pragma once

#include "abel/strings/format.h"
#include "abel/log/details/log_msg.h"

namespace abel {

class log_formatter {
  public:
    virtual ~log_formatter() = default;

    virtual void format(const details::log_msg &msg, memory_buf_t &dest) = 0;

    virtual std::unique_ptr<log_formatter> clone() const = 0;
};
}  // namespace abel
