//
// Created by liyinbin on 2021/4/5.
//

#ifndef ABEL_FIBER_CONDITION_VARIABLE_H_
#define ABEL_FIBER_CONDITION_VARIABLE_H_


#include <condition_variable>

#include "abel/fiber/internal/waitable.h"
#include "abel/fiber/fiber_mutex.h"

namespace abel {

    // Analogous to `std::condition_variable`, but it's for fiber.
    class fiber_cond {
    public:
        // Wake up one waiter.
        void notify_one() noexcept;

        // Wake up all waiters.
        void notify_all() noexcept;

        // Wait until someone called `notify_xxx()`.
        void wait(std::unique_lock<fiber_mutex>& lock);

        // Wait until `pred` is satisfied.
        template <class Predicate>
        inline void wait(std::unique_lock<fiber_mutex>& lock, Predicate pred);

        // Wait until either someone notified us or `expires_in` has expired.
        //
        // I totally have no idea why Standard Committee didn't simply specify this
        // method to return `bool` instead.
        inline std::cv_status wait_for(std::unique_lock<fiber_mutex>& lock,
                                const abel::duration& expires_in);

        // Wait until either `pred` is satisfied or `expires_in` has expired.
        template <class Predicate>
        inline bool wait_for(std::unique_lock<fiber_mutex>& lock,
                      const abel::duration& expires_in,
                      Predicate pred);


        // Wait until either `pred` is satisfied or `expires_at` is reached.
        template <class Pred>
        inline std::cv_status wait_until(std::unique_lock<fiber_mutex>& lock,
                        const abel::time_point& expires_at,
                        Pred pred);

        inline bool wait_until(std::unique_lock<fiber_mutex>& lock,
                        const abel::time_point& expires_at);

    private:
        fiber_internal::fiber_cond impl_;
    };

    template <class Predicate>
    inline void fiber_cond::wait(std::unique_lock<fiber_mutex>& lock, Predicate pred) {
        impl_.wait(lock, pred);
    }

    inline std::cv_status fiber_cond::wait_for(
            std::unique_lock<fiber_mutex>& lock,
            const abel::duration& expires_in) {
        auto steady_timeout = abel::time_now() + expires_in;
        return impl_.wait_until(lock, steady_timeout) ? std::cv_status::no_timeout
                                                      : std::cv_status::timeout;
    }

    template <class Predicate>
    inline bool fiber_cond::wait_for(
            std::unique_lock<fiber_mutex>& lock,
            const abel::duration& expires_in, Predicate pred) {
        auto steady_timeout = abel::time_now() + expires_in;
        return impl_.wait_until(lock, steady_timeout, pred);
    }

    inline bool fiber_cond::wait_until(
            std::unique_lock<fiber_mutex>& lock,
            const abel::time_point& expires_at) {
        return impl_.wait_until(lock, expires_at);
    }

    template <class Predicate>
    inline std::cv_status fiber_cond::wait_until(
            std::unique_lock<fiber_mutex>& lock,
            const abel::time_point& expires_at, Predicate pred) {
        return impl_.wait_until(lock, expires_at, pred) ? std::cv_status::no_timeout
                                                  : std::cv_status::timeout;
    }

}  // namespace abel

#endif  // ABEL_FIBER_CONDITION_VARIABLE_H_
