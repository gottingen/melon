//
// Created by liyinbin on 2021/4/5.
//

#include "abel/fiber/fiber_cond.h"

namespace abel {

    void fiber_cond::notify_one() noexcept { impl_.notify_one(); }

    void fiber_cond::notify_all() noexcept { impl_.notify_all(); }

    void fiber_cond::wait(std::unique_lock<fiber_mutex> &lock) {
        impl_.wait(lock);
    }

}  // namespace abel
