//
// Created by liyinbin on 2021/4/5.
//

#ifndef TESTING_FIBER_H_
#define TESTING_FIBER_H_


#include <atomic>
#include <thread>
#include <utility>

#include "abel/fiber/internal/fiber_entity.h"
#include "abel/fiber/internal/scheduling_group.h"
#include "abel/fiber/fiber.h"
#include "abel/fiber/runtime.h"

namespace testing {

    template <class F>
    void RunAsFiber(F&& f) {
        abel::start_runtime();
        std::atomic<bool> done{};
        abel::fiber([&, f = std::forward<F>(f)] {
            f();
            done = true;
        }).detach();
        while (!done) {
            std::this_thread::sleep_for(std::chrono::microseconds(1));
        }
        abel::terminate_runtime();
    }

    template <class F>
    void StartFiberEntityInGroup(abel::fiber_internal::scheduling_group* sg, bool system_fiber,
                                 F&& f) {
        auto fiber = abel::fiber_internal::create_fiber_entity(sg, system_fiber, std::forward<F>(f));
        fiber->scheduling_group_local = false;
        sg->ready_fiber(fiber, {});
    }

}  // namespace testing

#endif  // TESTING_FIBER_H_
