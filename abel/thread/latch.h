//
// Created by liyinbin on 2021/4/4.
//

#ifndef ABRL_THREAD_LATCH_H_
#define ABRL_THREAD_LATCH_H_

#include <condition_variable>
#include <mutex>

#include "abel/log/logging.h"

namespace abel {

    class latch {
    public:
        explicit latch(std::ptrdiff_t count);

        // Decrement internal counter. If it reaches zero, wake up all waiters.
        void count_down(std::ptrdiff_t update = 1);

        // Test if the latch's internal counter has become zero.
        bool try_wait() const noexcept;

        // Wait until `latch`'s internal counter reached zero.
        void wait() const;

        // Extension to `std::latch`.
        template<class Rep, class Period>
        bool wait_for(const std::chrono::duration<Rep, Period> &timeout) {
            std::unique_lock lk(m_);
            DCHECK_GE(count_, 0);
            return cv_.wait_for(lk, timeout, [this] { return count_ == 0; });
        }

        // Extension to `std::latch`.
        template<class Clock, class Duration>
        bool wait_until(const std::chrono::time_point<Clock, Duration> &timeout) {
            std::unique_lock lk(m_);
            DCHECK_GE(count_, 0);
            return cv_.wait_until(lk, timeout, [this] { return count_ == 0; });
        }

        // Shorthand for `count_down(); wait();`
        void arrive_and_wait(std::ptrdiff_t update = 1);

    private:
        mutable std::mutex m_;
        mutable std::condition_variable cv_;
        std::ptrdiff_t count_;
    };

}  // namespace abel

#endif  // ABRL_THREAD_LATCH_H_
