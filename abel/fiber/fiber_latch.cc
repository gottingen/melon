//
// Created by liyinbin on 2021/4/5.
//


#include "abel/fiber/fiber_latch.h"

namespace abel {

    fiber_latch::fiber_latch(std::ptrdiff_t count) : count_(count) {}

    void fiber_latch::count_down(std::ptrdiff_t update) {
        std::scoped_lock _(lock_);
        DCHECK_GE(count_, update);
        count_ -= update;
        if (!count_) {
            cv_.notify_all();
        }
    }

    bool fiber_latch::try_wait() const noexcept {
        std::scoped_lock _(lock_);
        DCHECK_GE(count_, 0);
        return !count_;
    }

    void fiber_latch::wait() const {
        std::unique_lock lk(lock_);
        DCHECK_GE(count_, 0);
        cv_.wait(lk, [&] { return count_ == 0; });
    }

    void fiber_latch::arrive_and_wait(std::ptrdiff_t update) {
        count_down(update);
        wait();
    }

}  // namespace abel
