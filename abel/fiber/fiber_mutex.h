//
// Created by liyinbin on 2021/4/5.
//

#ifndef ABEL_FIBER_FIBER_MUTEX_H_
#define ABEL_FIBER_FIBER_MUTEX_H_

#include "abel/fiber/internal/waitable.h"

namespace abel {

    // Analogous to `std::mutex`, but it's for fiber.
    using fiber_mutex = fiber_internal::fiber_mutex;

}  // namespace abel

#endif //ABEL_FIBER_FIBER_MUTEX_H_
