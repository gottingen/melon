//
// Created by liyinbin on 2020/3/22.
//

#ifndef ABEL_SCHEDULE_PREEMPT_H_
#define ABEL_SCHEDULE_PREEMPT_H_

#include <abel/base/profile.h>
#include <atomic>

namespace abel {


    extern __thread bool g_need_preempt;

    inline bool need_preempt() {
#ifndef ABEL_DEBUG
        // prevent compiler from eliminating loads in a loop
        std::atomic_signal_fence(std::memory_order_seq_cst);
        return ABEL_UNLIKELY(g_need_preempt);
#else
        return true;
#endif
    }

} //namespace abel

#endif //ABEL_SCHEDULE_PREEMPT_H_
