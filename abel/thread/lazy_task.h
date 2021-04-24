//
// Created by liyinbin on 2021/4/5.
//

#ifndef ABEL_THREAD_LAZY_TASK_H_
#define ABEL_THREAD_LAZY_TASK_H_


#include <chrono>
#include <cinttypes>

#include "abel/functional/function.h"
#include "abel/chrono/clock.h"

namespace abel {

    std::uint64_t set_thread_lazy_task(abel::function<void()> callback,
                                             abel::duration min_interval);

    void delete_thread_lazy_task(std::uint64_t handle);

    void notify_thread_lazy_task();

}  // namespace abel


#endif  // ABEL_THREAD_LAZY_TASK_H_
