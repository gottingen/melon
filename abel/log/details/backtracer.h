// Copyright(c) 2015-present, Gabi Melman & spdlog contributors.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#pragma once

#include <atomic>
#include <mutex>
#include <functional>
#include "abel/log/details/log_msg_buffer.h"
#include "abel/log/details/circular_q.h"

// Store log messages in circular buffer.
// Useful for storing debug data in case of error/warning happens.

namespace abel {
namespace details {
class ABEL_API backtracer {
    mutable std::mutex mutex_;
    std::atomic<bool> enabled_{false};
    circular_q<log_msg_buffer> messages_;

  public:
    backtracer() = default;

    backtracer(const backtracer &other);

    backtracer(backtracer &&other) ABEL_NOEXCEPT;

    backtracer &operator=(backtracer other);

    void enable(size_t size);

    void disable();

    bool enabled() const;

    void push_back(const log_msg &msg);

    // pop all items in the q and apply the given fun on each of them.
    void foreach_pop(std::function<void(const details::log_msg &)> fun);
};

} // namespace details
}  // namespace abel

#include "abel/log/details/backtracer_inl.h"
