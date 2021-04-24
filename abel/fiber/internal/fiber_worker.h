//
// Created by liyinbin on 2021/4/4.
//

#ifndef ABEL_FIBER_INTERNAL_FIBER_WORKER_H_
#define ABEL_FIBER_INTERNAL_FIBER_WORKER_H_

#include <cstddef>

#include <queue>
#include <thread>
#include "abel/thread/thread.h"
#include "abel/base/profile.h"

namespace abel {

    namespace fiber_internal {

        struct fiber_entity;
        class scheduling_group;

        // A pthread worker for running fibers.
        class alignas(hardware_destructive_interference_size) fiber_worker {
        public:
            fiber_worker(scheduling_group* sg, std::size_t worker_index);

            // Add foreign scheduling group for stealing.
            //
            // May only be called prior to `start()`.
            void add_foreign_scheduling_group(scheduling_group* sg,
                                           std::uint64_t steal_every_n);

            // start the worker thread.
            //
            // If `no_cpu_migration` is set, this fiber worker is bind to
            // #`worker_index`-th processor in `affinity`.
            void start(bool no_cpu_migration);

            // Wail until this worker quits.
            //
            // Note that there's no `Stop()` here. Instead, you should call
            // `scheduling_group::Stop()` to stop all the workers when exiting.
            void join();

            // Non-copyable, non-movable.
            fiber_worker(const fiber_worker&) = delete;
            fiber_worker& operator=(const fiber_worker&) = delete;

        private:
            void worker_proc();
            fiber_entity* steal_fiber();

        private:
            struct victim {
                scheduling_group* sg;
                std::uint64_t steal_every_n;
                std::uint64_t next_steal;

                // `std::priority_queue` orders elements descendingly.
                bool operator<(const victim& v) const { return next_steal > v.next_steal; }
            };

            scheduling_group* sg_;
            std::size_t worker_index_;
            std::uint64_t steal_vec_clock_{};
            std::priority_queue<victim> victims_;
            abel::thread worker_;
        };

    }  // namespace fiber_internal
}  // namespace abel

#endif  // ABEL_FIBER_INTERNAL_FIBER_WORKER_H_
