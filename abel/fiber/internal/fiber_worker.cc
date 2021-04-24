//
// Created by liyinbin on 2021/4/4.
//

#include <thread>

#include "abel/fiber/internal/fiber_worker.h"
#include "abel/fiber/internal/scheduling_group.h"
#include "abel/fiber/internal/fiber_entity.h"
#include "abel/thread/lazy_task.h"
#include "abel/base/random.h"
#include "abel/thread/thread.h"

namespace abel {
    namespace fiber_internal {
        fiber_worker::fiber_worker(scheduling_group *sg, std::size_t worker_index)
                : sg_(sg), worker_index_(worker_index) {}

        void fiber_worker::add_foreign_scheduling_group(scheduling_group *sg,
                                                        std::uint64_t steal_every_n) {
            victims_.push({.sg = sg,
                                  .steal_every_n = steal_every_n,
                                  .next_steal = abel::Random(steal_every_n)});
        }

        void fiber_worker::start(bool no_cpu_migration) {

            auto group_affinity = sg_->affinity();
            DCHECK(!no_cpu_migration || group_affinity.count() == 0);
            core_affinity affinity;
            if (group_affinity.count() > 0 && no_cpu_migration) {
                DCHECK(worker_index_ < group_affinity.count());

                affinity.add(core_affinity{group_affinity[worker_index_]});

                DLOG_INFO("Fiber worker #{} is started on dedicated processsor #{}.",
                          worker_index_, affinity[0].index);
            }

            worker_ = abel::thread(std::move(affinity), [this] {
                worker_proc();
            });
        }

        void fiber_worker::join() { worker_.join(); }

        void fiber_worker::worker_proc() {
            sg_->enter_group(worker_index_);

            while (true) {
                auto fiber = sg_->acquire_fiber();

                if (!fiber) {
                    fiber = sg_->spinning_acquire_fiber();
                    if (!fiber) {
                        fiber = steal_fiber();
                        DCHECK_NE(fiber, scheduling_group::kSchedulingGroupShuttingDown);
                        if (!fiber) {
                            fiber = sg_->wait_for_fiber();  // This one either sleeps, or succeeds.
                            // `DCHECK_NE` does not handle `nullptr` well.
                            DCHECK_NE(fiber, static_cast<fiber_entity *>(nullptr));
                        }
                    }
                }

                if (ABEL_UNLIKELY(fiber ==
                                  scheduling_group::kSchedulingGroupShuttingDown)) {
                    break;
                }

                fiber->resume();

                // Notify the framework that any pending operations can be performed.
                //TODO
                notify_thread_lazy_task();
            }
            DCHECK_EQ(get_current_fiber_entity(), get_master_fiber_entity());
            sg_->leave_group();
        }

        fiber_entity *fiber_worker::steal_fiber() {
            if (victims_.empty()) {
                return nullptr;
            }

            ++steal_vec_clock_;
            while (victims_.top().next_steal <= steal_vec_clock_) {
                auto &&top = victims_.top();
                if (auto rc = top.sg->remote_acquire_fiber()) {
                    // We don't pop the top in this case, since it's not empty, maybe the next
                    // time we try to steal, there are still something for us.
                    return rc;
                }
                victims_.push({.sg = top.sg,
                                      .steal_every_n = top.steal_every_n,
                                      .next_steal = top.next_steal + top.steal_every_n});
                victims_.pop();
                // Try next victim then.
            }
            return nullptr;
        }

    }  // namespace fiber_internal
}  // namespace abel