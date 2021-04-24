//
// Created by liyinbin on 2021/4/4.
//

#ifndef ABEL_FIBER_INTERNAL_TIMER_WORKER_H_
#define ABEL_FIBER_INTERNAL_TIMER_WORKER_H_

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

#include "abel/thread/thread.h"
#include "abel/base/profile.h"
#include "abel/functional/function.h"
#include "abel/thread/latch.h"
#include "abel/memory/ref_ptr.h"

namespace abel {

    template<class>
    struct pool_traits;

}  // namespace abel

namespace abel {

    namespace fiber_internal {

        class scheduling_group;

        // This class contains a dedicated pthread for running timers.
        class alignas(hardware_destructive_interference_size) timer_worker {
        public:
            explicit timer_worker(scheduling_group *sg);

            ~timer_worker();

            static timer_worker *get_timer_owner(std::uint64_t timer_id);

            // Create a timer. It's enabled separately via `enable_timer`.
            //
            // @sa: `scheduling_group::create_timer`
            std::uint64_t create_timer(abel::time_point expires_at,
                                      abel::function<void(std::uint64_t)> &&cb);

            std::uint64_t create_timer(abel::time_point initial_expires_at,
                    abel::duration interval, abel::function<void(std::uint64_t)> &&cb);

            // Enable a timer created before.
            void enable_timer(std::uint64_t timer_id);

            // Cancel a timer.
            void remove_timer(std::uint64_t timer_id);

            // Detach a timer. This method can be helpful in fire-and-forget use cases.
            void detach_timer(std::uint64_t timer_id);

            scheduling_group *get_scheduling_group();

            // Caller MUST be one of the pthread workers belong to the same scheduling
            // group.
            void initialize_local_queue(std::size_t worker_index);

            // Start the worker thread and join the given scheduling group.
            void start();

            // Stop & Join.
            void stop();

            void join();

            // Non-copyable, non-movable.
            timer_worker(const timer_worker &) = delete;

            timer_worker &operator=(const timer_worker &) = delete;

        private:
            struct Entry;
            using EntryPtr = ref_ptr<Entry>;
            struct ThreadLocalQueue;

            void worker_proc();

            void add_timer(EntryPtr timer);

            // Wait until all worker has called `InitializeLocalQueue()`.
            void wait_for_workers();

            void reap_thread_local_queues();

            void fire_timers();

            void wake_worker_if_needed(abel::time_point local_expires_at);

            static ThreadLocalQueue *get_thread_local_queue();

        private:
            struct EntryPtrComp {
                bool operator()(const EntryPtr &p1, const EntryPtr &p2) const;
            };

            friend struct pool_traits<Entry>;

            std::atomic<bool> stopped_{false};
            scheduling_group *sg_;
            abel::latch latch;  // We use it to wait for workers' registration.

            // Pointer to thread-local timer vectors.
            std::vector<ThreadLocalQueue *> producers_;

            std::atomic<int64_t> next_expires_at_{std::numeric_limits<int64_t>::max()};
            std::priority_queue<EntryPtr, std::vector<EntryPtr>, EntryPtrComp> timers_;

            abel::thread worker_;

            // `WorkerProc` sleeps on this.
            std::mutex lock_;
            std::condition_variable cv_;
        };

    }  // namespace fiber_internal
}  // namespace abel
#endif  // ABEL_FIBER_INTERNAL_TIMER_WORKER_H_
