//
// Created by liyinbin on 2021/4/5.
//

#include "abel/fiber/this_fiber.h"

#include "abel/fiber/internal/fiber_entity.h"
#include "abel/fiber/internal/scheduling_group.h"
#include "abel/fiber/internal/waitable.h"

namespace abel {

    void fiber_yield() {
        auto self = fiber_internal::get_current_fiber_entity();
        DCHECK_MSG(self,
                    "this_fiber::yield may only be called in fiber environment.");
        self->own_scheduling_group->yield(self);
    }

    void fiber_sleep_until(const abel::time_point& expires_at) {
        abel::fiber_internal::waitable_timer wt(expires_at);
        wt.wait();
    }

    void fiber_sleep_for(const abel::duration& expires_in) {
        fiber_sleep_until(abel::time_now() + expires_in);
    }

}  // namespace abel