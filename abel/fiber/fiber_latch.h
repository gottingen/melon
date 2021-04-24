//
// Created by liyinbin on 2021/4/5.
//

#ifndef ABEL_FIBER_FIBER_LATCH_H_
#define ABEL_FIBER_FIBER_LATCH_H_


#include "abel/functional/function.h"
#include "abel/fiber/fiber_mutex.h"
#include "abel/fiber/fiber_cond.h"
#include "abel/fiber/fiber_mutex.h"

namespace abel {

    // Analogous to `std::latch`, but it's for fiber.
    //
    // TODO(yinbinli): We do not perfectly match `std::latch` yet.
    class fiber_latch {
    public:
        // TODO(yinbinli): static constexpr ptrdiff_t max() noexcept;

        explicit fiber_latch(std::ptrdiff_t count);

        // Count the latch down.
        //
        // If total number of call to this method reached `count` (passed to
        // constructor), all waiters are waken up.
        //
        // `std::latch` support specifying count here, we don't support it for the
        // moment.
        void count_down(std::ptrdiff_t update = 1);

        // Test if the latch's internal counter has become zero.
        bool try_wait() const noexcept;

        // Wait until the latch's internal counter becomes zero.
        void wait() const;

        // Extension to `std::latch`.
        template <class Rep, class Period>
        bool wait_for(const std::chrono::duration<Rep, Period>& timeout) {
            std::unique_lock lk(lock_);
            DCHECK_GE(count_, 0);
            return cv_.wait_for(lk, timeout, [this] { return count_ == 0; });
        }

        // Extension to `std::latch`.
        template <class Clock, class Duration>
        bool wait_until(const std::chrono::time_point<Clock, Duration>& timeout) {
            std::unique_lock lk(lock_);
            DCHECK_GE(count_, 0);
            return cv_.wait_until(lk, timeout, [this] { return count_ == 0; });
        }

        // Count the latch down and wait for it to become zero.
        //
        // If total number of call to this method reached `count` (passed to
        // constructor), all waiters are waken up.
        void arrive_and_wait(std::ptrdiff_t update = 1);

    private:
        mutable fiber_mutex lock_;
        mutable fiber_cond cv_;
        std::ptrdiff_t count_;
    };

}  // namespace abel


#endif  // ABEL_FIBER_FIBER_LATCH_H_
