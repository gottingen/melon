//
// Created by liyinbin on 2021/4/4.
//

#ifndef ABEL_FIBER_INTERNAL_SCHEDULING_GROUP_H_
#define ABEL_FIBER_INTERNAL_SCHEDULING_GROUP_H_

#include "abel/fiber/internal/run_queue.h"
#include "abel/fiber/internal/timer_worker.h"
#include "abel/fiber/internal/spin_lock.h"
#include "abel/thread/thread.h"
#include "abel/base/profile.h"
#include "abel/base/annotation.h"

namespace abel {
    namespace fiber_internal {

        struct fiber_entity;

        class timer_worker;

        // Each scheduling group consists of a group of pthread worker and exactly one
        // timer worker (who is responsible for, timers).
        //
        // `scheduling_group` itself is merely a data structure, it's `fiber_worker` &
        // `timer_worker`'s responsibility for running fibers / timers.
        class alignas(hardware_destructive_interference_size) scheduling_group {
        public:
            // Guard value for marking the scheduling group is shutting down.
            inline static fiber_entity *const kSchedulingGroupShuttingDown =
                    reinterpret_cast<fiber_entity *>(0x1);

            // Worker index for timer worker.
            inline static constexpr std::size_t kTimerWorkerIndex = -1;

            // Construct a scheduling group consisting of `size` pthread workers (not
            // including the timer worker.).
            explicit scheduling_group(const core_affinity &affinity, std::size_t size);

            // We define destructor in `.cc` so as to use incomplete type in member
            // fields.
            ~scheduling_group();

            // Get current scheduling group.
            static scheduling_group *current() noexcept { return current_; }

            // Get scheduling group the given timer belongs to.
            static scheduling_group *get_timer_owner(std::uint64_t timer_id) noexcept {
                return timer_worker::get_timer_owner(timer_id)->get_scheduling_group();
            }

            // Acquire a (ready) fiber to run. Any memory modification done by the fiber
            // returned when it's pushed into scheduling queue (by `ready_fiber`) are
            // visible to the caller.
            //
            // Returns `nullptr` if there's none.
            //
            // Returns `kSchedulingGroupShuttingDown` if the entire scheduling group is
            // shutting down *and* there's no ready fiber to run.
            fiber_entity *acquire_fiber() noexcept;

            // Spin and try to acquire a fiber.
            fiber_entity *spinning_acquire_fiber() noexcept;

            // Sleep until at least one fiber is ready for run or the entire scheduling
            // group is shutting down.
            //
            // The method can return spuriously, i.e., return `nullptr` even if it should
            // keep sleeping instead.
            fiber_entity *wait_for_fiber() noexcept;

            // Acquire a fiber. The calling thread does not belong to this scheduling
            // group (i.e., it's stealing a fiber.).
            //
            // Returns `nullptr` if there's none. This method never returns
            // `kSchedulingGroupShuttingDown`.
            fiber_entity *remote_acquire_fiber() noexcept;

            // Schedule fibers in [start, end) to run in batch.
            //
            // No scheduling lock should be held by the caller, and all fibers to be
            // scheduled mustn't be run before (i.e., this is the fiber time they're
            // pushed into run queue.).
            //
            // Provided for perf. reasons. The same behavior can be achieved by calling
            // `ready_fiber` multiple times.
            //
            // CAUTION: `scheduling_group_local` is NOT respected by this method. (FIXME.)
            //
            // TODO(yinbinli): `span<fiber_entity*>` seems to be superior.
            void start_fibers(fiber_entity **start, fiber_entity **end) noexcept;

            // Schedule a fiber to run.
            //
            // `fiber_entity::scheduler_lock` of `fiber` must be held by the caller. This
            // helps you prevent race between call to this method and call to `halt()`.
            //
            // Special case: `scheduler_lock` can be empty if `fiber` has never been run,
            // in this case there is no race possible that should be prevented by
            // `scheduler_lock`.
            void ready_fiber(fiber_entity *fiber,
                            std::unique_lock <abel::fiber_internal::spinlock> &&scheduler_lock) noexcept;

            // halt caller fiber.
            //
            // The caller need to be woken by someone else explicitly via `ready_fiber`.
            //
            // `fiber_entity::scheduler_lock` of `fiber` must be held by the caller. This
            // helps you prevent race between call to this method and call to
            // `ready_fiber()`.
            //
            // `self` must be caller's `fiber_entity*`.
            void halt(fiber_entity *self,
                      std::unique_lock <abel::fiber_internal::spinlock> &&scheduler_lock) noexcept;

            // yield pthread worker to someone else.
            //
            // The caller must not be added to run queue by anyone else (either
            // concurrently or prior to this call). It will be added to run queue by this
            // method.
            //
            // `self->scheduler_lock` must NOT be held.
            //
            // The caller is automatically added to run queue by this method. (@sa:
            // `halt`).
            void yield(fiber_entity *self) noexcept;

            // yield pthread worker to the specified fiber.
            //
            // Both `self` and `to` must not be added to run queue by anyone else (either
            // concurrently or prior to this call). They'll be add to run queue by this
            // method.
            //
            // Scheduler lock of `self` and `to` must NOT be held.
            //
            // The caller is automatically added to run queue for run by other workers.
            void switch_to(fiber_entity *self, fiber_entity *to) noexcept;

            // Create a (not-scheduled-yet) timer. You must enable it later via
            // `enable_timer`.
            //
            // Timer ID returned is also given to timer callback on called.
            //
            // Timer ID returned must be either detached via `detach_timer` or freed (the
            // timer itself is cancelled in if freed) via `remove_timer`. Otherwise a leak
            // will occur.
            //
            // The reason why we choose a two-step way to set up a timer is that, in
            // certain cases, the timer's callback may want to access timer's ID stored
            // somewhere (e.g., some global data structure). If creating timer and
            // enabling it is done in a single step, the user will have a hard time to
            // synchronizes between timer-creator and timer-callback.
            //
            // This method can only be called inside **this** scheduling group's fiber
            // worker context.
            //
            // Note that `cb` is called in timer worker's context, you normally would want
            // to fire a fiber for run your own logic.
            [[nodiscard]] std::uint64_t create_timer(
                    abel::time_point expires_at,
                    abel::function<void(std::uint64_t)> &&cb) {
                DCHECK(timer_worker_);
                DCHECK_EQ(current(), this);
                return timer_worker_->create_timer(expires_at, std::move(cb));
            }

            // Periodic timer.
            [[nodiscard]] std::uint64_t create_timer(
                    abel::time_point initial_expires_at,
                    abel::duration interval, abel::function<void(std::uint64_t)> &&cb) {
                DCHECK(timer_worker_);
                DCHECK_EQ(current(), this);
                return timer_worker_->create_timer(initial_expires_at, interval,
                                                  std::move(cb));
            }

            // Enable a timer. Timer's callback can be called even before this method
            // returns.
            void enable_timer(std::uint64_t timer_id) {
                DCHECK(timer_worker_);
                DCHECK_EQ(current(), this);
                return timer_worker_->enable_timer(timer_id);
            }

            // Detach a timer.
            void detach_timer(std::uint64_t timer_id) noexcept {
                DCHECK(timer_worker_);
                return timer_worker_->detach_timer(timer_id);
            }

            // Cancel a timer set by `set_timer`.
            //
            // Callable in any thread belonging to the same scheduling group.
            //
            // If the timer is being fired (or has fired), this method does nothing.
            void remove_timer(std::uint64_t timer_id) noexcept {
                DCHECK(timer_worker_);
                return timer_worker_->remove_timer(timer_id);
            }

            // Workers (including timer worker) call this to join this scheduling group.
            //
            // Several thread-local variables used by `scheduling_group` is initialize
            // here.
            //
            // After calling this method, `Current` is usable.
            void enter_group(std::size_t index);

            // You're calling this on thread exit.
            void leave_group();

            // Get number of *pthread* workers (not including the timer worker) in this
            // scheduling group.
            std::size_t group_size() const noexcept;

            // CPU affinity of this scheduling group, or an empty vector if not specified.
            core_affinity affinity();

            // Set timer worker. This method must be called before registering pthread
            // workers.
            //
            // This worker is later used by `set_timer` for setting timers.
            //
            // `scheduling_group` itself guarantees if it calls methods on `worker`, the
            // caller pthread is one of the pthread worker in this scheduling group.
            // (i.e., the number of different caller thread equals to `group_size()`.
            //
            // On shutting down, the `timer_worker` must be joined before joining pthread
            // workers, otherwise use-after-free can occur.
            void set_timer_worker(timer_worker *worker) noexcept;

            // Shutdown the scheduling group.
            //
            // All further calls to `set_timer` / `DispatchFiber` leads to abort.
            //
            // Wake up all workers blocking on `wait_for_fiber`, once all ready fiber has
            // terminated, all further calls to `acquire_fiber` returns
            // `kSchedulingGroupShuttingDown`.
            //
            // It's still your responsibility to shut down pthread workers / timer
            // workers. This method only mark this control structure as being shutting
            // down.
            void stop();

        private:
            bool wake_up_one_worker() noexcept;

            bool wake_up_workers(std::size_t n) noexcept;

            bool wake_up_one_spinning_worker() noexcept;

            bool wake_up_one_deep_sleeping_worker() noexcept;

        private:
            static constexpr auto kUninitializedWorkerIndex =
                    std::numeric_limits<std::size_t>::max();
            ABEL_INTERNAL_TLS_MODEL static inline thread_local scheduling_group
            *
            current_;
            ABEL_INTERNAL_TLS_MODEL static thread_local std::size_t
            worker_index_;

            class wait_slot;

            std::atomic<bool> stopped_{false};
            std::size_t group_size_;
            timer_worker *timer_worker_ = nullptr;
            core_affinity affinity_;

            // Exposes internal state.
           // lazy_init <ExposedVarDynamic<std::string>> spinning_workers_var_,
//                    sleeping_workers_var_;

            // Ready fibers are put here.
            run_queue run_queue_;

            // Fiber workers sleep on this.
            std::unique_ptr<wait_slot[]> wait_slots_;

            // Bit mask.
            //
            // We carefully chose to use 1 to represent "spinning" and "sleeping", instead
            // of "running" and "awake", here. This way if the number of workers is
            // smaller than 64, those not-used bit is treated as if they represent running
            // workers, and don't need special treat.
            alignas(hardware_destructive_interference_size)
            std::atomic <std::uint64_t> spinning_workers_{0};
            alignas(hardware_destructive_interference_size)
            std::atomic <std::uint64_t> sleeping_workers_{0};

            // Set if the last spinner successfully grabbed a fiber to run. In this case
            // we're likely under load, so it set this flag for other spinners (if there
            // is one) to wake more workers up (and hopefully to get a fiber or spin).
            alignas(hardware_destructive_interference_size)
            std::atomic<bool> pending_spinner_wakeup_{false};
        };

    }  // namespace fiber_internal
}  // namespace abel
#endif  // ABEL_FIBER_INTERNAL_SCHEDULING_GROUP_H_
