//
// Created by liyinbin on 2021/4/4.
//

#include "abel/fiber/internal/run_queue.h"

#include <cstdint>
#include <utility>

#include "abel/base/profile.h"
#include "abel/log/logging.h"
#include "abel/fiber/internal/assembly.h"

namespace abel {
    namespace fiber_internal {

        run_queue::run_queue(std::size_t capacity)
                : capacity_(capacity), mask_(capacity_ - 1) {
            DCHECK((capacity & (capacity - 1)) == 0,
                        "Capacity must be a power of 2.");
            head_seq_.store(0, std::memory_order_relaxed);
            tail_seq_.store(0, std::memory_order_relaxed);
            nodes_ = std::make_unique<queue_node[]>(capacity_);
            for (std::size_t index = 0; index != capacity_; ++index) {
                nodes_[index].seq.store(index, std::memory_order_relaxed);  // `release`?
            }
        }

        run_queue::~run_queue() = default;

        bool run_queue::batch_push(fiber_entity** start, fiber_entity** end,
                                 bool instealable) {
            auto batch = end - start;
            while (true) {
                auto head_was = head_seq_.load(std::memory_order_relaxed);
                auto head = head_was + batch;
                auto hseq = nodes_[head & mask_].seq.load(std::memory_order_acquire);

                // Let's see if the last node we're trying to claim is not occupied.
                if (ABEL_LIKELY(hseq == head)) {
                    // First check if the entire range is clean.
                    for (int i = 0; i != batch; ++i) {
                        auto&& n = nodes_[(head_was + i) & mask_];
                        auto seq = n.seq.load(std::memory_order_acquire);
                        if (ABEL_UNLIKELY(seq != head_was + i)) {
                            if (seq + capacity_ == head_was + i + 1) {
                                // This node hasn't been fully reset. Bail out.
                                return false;
                            }  // Fall-through otherwise.
                        }
                    }

                    // Try claiming the entire range of [head_was, head).
                    if (ABEL_LIKELY(head_seq_.compare_exchange_weak(
                            head_was, head, std::memory_order_relaxed))) {
                        // Fill them then.
                        for (int i = 0; i != batch; ++i) {
                            auto&& n = nodes_[(head_was + i) & mask_];
                            DCHECK_EQ(n.seq.load(std::memory_order_relaxed), head_was + i);
                            n.fiber = start[i];
                            n.instealable.store(instealable, std::memory_order_relaxed);
                            n.seq.store(head_was + i + 1, std::memory_order_release);
                        }
                        // TODO(yinbinli): We can use `std::memory_order_relaxed` above when
                        // storing into `n.seq`, and do a release fence for all of them here.
                        return true;
                    }
                    // Fall-through otherwise.
                } else if (ABEL_UNLIKELY(hseq + capacity_ == head + 1)) {  // Overrun.
                    // @sa: Comments in `PushSlow`.
                    return false;
                } else {
                    // We've been too late, retry.
                }  // Fall-through.
                pause();
            }
        }

        fiber_entity* run_queue::steal() {
            return pop_if([](const queue_node& node) {
                return !node.instealable.load(std::memory_order_relaxed);
            });
        }

        fiber_entity* run_queue::pop_slow() {
            return pop_if([](auto&&) { return true; });
        }

        bool run_queue::push_slow(fiber_entity* e, bool instealable) {
            while (true) {
                auto head = head_seq_.load(std::memory_order_relaxed);
                auto&& n = nodes_[head & mask_];
                auto nseq = n.seq.load(std::memory_order_acquire);
                if (ABEL_LIKELY(nseq == head)) {
                    if (ABEL_LIKELY(head_seq_.compare_exchange_weak(
                            head, head + 1, std::memory_order_relaxed))) {
                        n.fiber = e;
                        n.instealable.store(instealable, std::memory_order_relaxed);
                        n.seq.store(head + 1, std::memory_order_release);
                        return true;
                    }
                    // Fall-through.
                } else if (ABEL_UNLIKELY(nseq + capacity_ == head + 1)) {  // Overrun.
                    // To whoever is debugging this code:
                    //
                    // You can see "false positive" if you set a breakpoint or call `abort`
                    // here.
                    //
                    // The reason is that the thread calling this method can be delayed
                    // arbitrarily long after loading `head_seq_` and `n.seq`, but before
                    // testing if `nseq + capacity_ == head + 1` holds. By the time the
                    // expression is tested, it's possible that the queue has indeed been
                    // emptied.
                    //
                    // Therefore, you can see this branch taken even if the queue is empty *at
                    // some point* during this method's execution.
                    //
                    // This should be expected and handled by the caller (presumably you, as
                    // you're debugging this method), though. The only thing a thread-safe
                    // method can guarantee is that, at **some** point, but not every point,
                    // during its call, its behavior conforms to what it's intended to do.
                    //
                    // Technically, this time point is called as the method's "linearization
                    // point". And this method is lineralized at the instant when `n.seq` is
                    // loaded.
                    return false;
                } else {
                    // We've been too late, retry.
                }  // Fall-through.
                pause();
            }
        }

        bool run_queue::unsafe_empty() const {
            return head_seq_.load(std::memory_order_relaxed) <=
                   tail_seq_.load(std::memory_order_relaxed);
        }

        template <class F>
        fiber_entity* run_queue::pop_if(F&& f) {
            while (true) {
                auto tail = tail_seq_.load(std::memory_order_relaxed);
                auto&& n = nodes_[tail & mask_];
                auto nseq = n.seq.load(std::memory_order_acquire);
                if (ABEL_LIKELY(nseq == tail + 1)) {
                    // Test before claim ownership.
                    if (!std::forward<F>(f)(n)) {
                        return nullptr;
                    }
                    if (ABEL_LIKELY(tail_seq_.compare_exchange_weak(
                            tail, tail + 1, std::memory_order_relaxed))) {
                        (void)n.seq.load(std::memory_order_acquire);  // We need this fence.
                        auto rc = n.fiber;
                        n.seq.store(tail + capacity_, std::memory_order_release);
                        return rc;
                    }
                } else if (ABEL_UNLIKELY(nseq == tail ||               // Not filled yet
                                          nseq + capacity_ == tail)) {  // Wrap around
                    // Underrun.
                    return nullptr;
                } else {
                    // Fall-through.
                }
                pause();
            }
        }
    }  // namespace fiber_internal
}