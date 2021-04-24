//
// Created by liyinbin on 2021/4/4.
//

#include "abel/thread/latch.h"


namespace abel {

    latch::latch(std::ptrdiff_t count) : count_(count) {}

    void latch::count_down(std::ptrdiff_t update) {
        std::unique_lock lk(m_);
        DCHECK_GE(count_, update);
        count_ -= update;
        if (!count_) {
            cv_.notify_all();
        }
    }

    bool latch::try_wait() const noexcept {
        std::scoped_lock _(m_);
        DCHECK_GE(count_, 0);
        return !count_;
    }

    void latch::wait() const {
        std::unique_lock lk(m_);
        DCHECK_GE(count_, 0);
        return cv_.wait(lk, [this] { return count_ == 0; });
    }

    void latch::arrive_and_wait(std::ptrdiff_t update) {
        count_down(update);
        wait();
    }

}  // namespace abel
