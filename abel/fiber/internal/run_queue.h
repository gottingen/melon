//
// Created by liyinbin on 2021/4/4.
//

#ifndef ABEL_FIBER_INTERNAL_RUN_QUEUE_H_
#define ABEL_FIBER_INTERNAL_RUN_QUEUE_H_

#include <cstddef>

#include <atomic>
#include <memory>

#include "abel/base/profile.h"

namespace abel {
    namespace fiber_internal {

        struct fiber_entity;

        // Thread-safe queue for storing runnable fibers.
        class alignas(hardware_destructive_interference_size) run_queue {
        public:
            // Initialize a queue whose capacity is `capacity`.
            //
            // `capacity` must be a power of 2.
            explicit run_queue(std::size_t capacity);

            // Destroy the queue.
            ~run_queue();

            // push a fiber into the run queue.
            //
            // `instealable` should be `e.scheduling_group_local`. Internally we store
            // this value separately for `Steal` to use. This is required since `Steal`
            // cannot access `fiber_entity` without claim ownership of it. In the meantime,
            // once the ownership is claimed (and subsequently to find the `fiber_entity`
            // cannot be stolen), it can't be revoked easily. So we treat `fiber_entity` as
            // opaque, and avoid access `fiber_entity` it at all.
            //
            // Returns `false` on overrun.
            bool push(fiber_entity *e, bool instealable) {
                auto head = head_seq_.load(std::memory_order_relaxed);
                auto &&n = nodes_[head & mask_];
                auto nseq = n.seq.load(std::memory_order_acquire);
                if (ABEL_LIKELY(nseq == head &&
                                 head_seq_.compare_exchange_strong(
                                         head, head + 1, std::memory_order_relaxed))) {
                    n.fiber = e;
                    n.instealable.store(instealable, std::memory_order_relaxed);
                    n.seq.store(head + 1, std::memory_order_release);
                    return true;
                }
                return push_slow(e, instealable);
            }

            // Push fibers in batch into the run queue.
            //
            // Returns `false` on overrun.
            bool batch_push(fiber_entity **start, fiber_entity **end, bool instealable);

            // Pop a fiber from the run queue.
            //
            // Returns `nullptr` if the queue is empty.
            fiber_entity *pop() {
                auto tail = tail_seq_.load(std::memory_order_relaxed);
                auto &&n = nodes_[tail & mask_];
                auto nseq = n.seq.load(std::memory_order_acquire);
                if (ABEL_LIKELY(nseq == tail + 1 &&
                                 tail_seq_.compare_exchange_strong(
                                         tail, tail + 1, std::memory_order_relaxed))) {
                    (void) n.seq.load(std::memory_order_acquire);  // We need this fence.
                    auto rc = n.fiber;
                    n.seq.store(tail + capacity_, std::memory_order_release);
                    return rc;
                }
                return pop_slow();
            }

            // Steal a fiber from this run queue.
            //
            // If the first fibers in the queue was pushed with `instealable` set,
            // `nullptr` will be returned.
            //
            // Returns `nullptr` if the queue is empty.
            fiber_entity *steal();

            // Test if the queue is empty. The result might be inaccurate.
            bool unsafe_empty() const;

        private:
            struct alignas(hardware_destructive_interference_size) queue_node {
                fiber_entity *fiber;
                std::atomic<bool> instealable;
                std::atomic<std::uint64_t> seq;
            };

            bool push_slow(fiber_entity *e, bool instealable);

            fiber_entity *pop_slow();

            template<class F>
            fiber_entity *pop_if(F &&f);

            std::size_t capacity_;
            std::size_t mask_;
            std::unique_ptr<queue_node[]> nodes_;
            alignas(hardware_destructive_interference_size)
            std::atomic<std::size_t> head_seq_;
            alignas(hardware_destructive_interference_size)
            std::atomic<std::size_t> tail_seq_;
        };

    }  // namespace fiber_internal
}  // namespace abel
#endif  // ABEL_FIBER_INTERNAL_RUN_QUEUE_H_
