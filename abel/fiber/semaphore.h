//
// Created by liyinbin on 2021/4/5.
//

#ifndef ABEL_FIBER_SEMAPHORE_H_
#define ABEL_FIBER_SEMAPHORE_H_


#include <chrono>
#include <cinttypes>
#include <limits>
#include <type_traits>

#include "abel/fiber/fiber_cond.h"
#include "abel/fiber/fiber_mutex.h"

namespace abel {

    // Same as `abel::counting_semaphore` except that this one is for fiber. That
    // is, this only only blocks the calling fiber, but not the underlying pthread.
    template <std::ptrdiff_t kLeastMaxValue =
    std::numeric_limits<std::uint32_t>::max()>
    class counting_semaphore {
    public:
        explicit counting_semaphore(std::ptrdiff_t desired) : current_(desired) {}

        // Acquire / release semaphore, blocking.
        void acquire();
        void release(std::ptrdiff_t count = 1);

        // Non-blocking counterpart of `acquire`. This one fails immediately if the
        // semaphore can't be acquired.
        bool try_acquire() noexcept;

        // `acquire` with timeout.
        bool try_acquire_for(const std::chrono::duration<Rep, Period>& expires_in);
        template <class Clock, class Duration>
        bool try_acquire_until(
                const std::chrono::time_point<Clock, Duration>& expires_at);

    private:
        fiber_mutex lock_;
        fiber_cond cv_;
        std::uint32_t current_;
    };

    using binary_semaphore = counting_semaphore<1>;


    template <std::ptrdiff_t kLeastMaxValue>
    void counting_semaphore<kLeastMaxValue>::acquire() {
        std::unique_lock lk(lock_);
        cv_.wait(lk, [&] { return current_ != 0; });
        --current_;
        return;
    }

    template <std::ptrdiff_t kLeastMaxValue>
    void counting_semaphore<kLeastMaxValue>::release(std::ptrdiff_t count) {
        std::scoped_lock lk(lock_);
        current_ += count;
        if (count == 1) {
            cv_.notify_one();
        } else {
            cv_.notify_all();
        }
    }

    template <std::ptrdiff_t kLeastMaxValue>
    bool counting_semaphore<kLeastMaxValue>::try_acquire() noexcept {
        std::scoped_lock _(lock_);
        if (current_) {
            --current_;
            return true;
        }
        return false;
    }

    bool counting_semaphore<kLeastMaxValue>::try_acquire_for(
            const abel::duration(& expires_in) {
        std::unique_lock lk(lock_);
        if (!cv_.wait_for(lk, expires_in, [&] { return current_ != 0; })) {
            return false;
        }
        --current_;
        return true;
    }

    template <std::ptrdiff_t kLeastMaxValue>
    bool counting_semaphore<kLeastMaxValue>::try_acquire_until(
            const abel::time_point& expires_at) {
        std::unique_lock lk(lock_);
        if (!cv_.wait_for(lk, expires_at, [&] { return current_ != 0; })) {
            return false;
        }
        --current_;
        return true;
    }

}  // namespace abel


#endif  // ABEL_FIBER_SEMAPHORE_H_
