
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef MELON_FIBER_INTERNAL_PARKING_LOT_H_
#define MELON_FIBER_INTERNAL_PARKING_LOT_H_

#include "melon/base/static_atomic.h"
#include "melon/fiber/internal/sys_futex.h"

namespace melon::fiber_internal {

    // Park idle workers.
    class MELON_CACHELINE_ALIGNMENT ParkingLot {
    public:
        class State {
        public:
            State() : val(0) {}

            bool stopped() const { return val & 1; }

        private:

            friend class ParkingLot;

            State(int val) : val(val) {}

            int val;
        };

        ParkingLot() : _pending_signal(0) {}

        // Wake up at most `num_task' workers.
        // Returns #workers woken up.
        int signal(int num_task) {
            _pending_signal.fetch_add((num_task << 1), std::memory_order_release);
            return futex_wake_private(&_pending_signal, num_task);
        }

        // Get a state for later wait().
        State get_state() {
            return _pending_signal.load(std::memory_order_acquire);
        }

        // Wait for tasks.
        // If the `expected_state' does not match, wait() may finish directly.
        void wait(const State &expected_state) {
            futex_wait_private(&_pending_signal, expected_state.val, NULL);
        }

        // Wakeup suspended wait() and make them unwaitable ever.
        void stop() {
            _pending_signal.fetch_or(1);
            futex_wake_private(&_pending_signal, 10000);
        }

    private:
        // higher 31 bits for signalling, LSB for stopping.
        std::atomic<int> _pending_signal;
    };

}  // namespace melon::fiber_internal

#endif  // MELON_FIBER_INTERNAL_PARKING_LOT_H_
