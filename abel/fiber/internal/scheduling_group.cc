//
// Created by liyinbin on 2021/4/4.
//


#include "abel/fiber/internal/scheduling_group.h"

#if defined(ABEL_PLATFORM_LINUX)
#include <linux/futex.h>
#endif
#include <sys/mman.h>
#include <sys/syscall.h>
#include <sys/wait.h>

#include <climits>
#include <memory>
#include <string>
#include <thread>

#include "gflags/gflags.h"

#include "abel/base/profile.h"
#include "abel/functional/function.h"
#include "abel/base/annotation.h"
#include "abel/thread/latch.h"
#include "abel/memory/object_pool.h"
#include "abel/base/random.h"
#include "abel/fiber/internal/assembly.h"
#include "abel/fiber/internal/fiber_entity.h"
#include "abel/fiber/internal/timer_worker.h"
#include "abel/fiber/internal/waitable.h"

using namespace std::literals;

// Note that unless the user do not enable guard page (in stack), there's always
// a limit on maximum alive (not only runnable) fibers in the process.
//
// @sa: `StackAllocator` for more details.
DEFINE_int32( fiber_run_queue_size, 65536,
          "Maximum runnable fibers per scheduling group. This value must be "
          "a power of 2.");

namespace abel {
    namespace fiber_internal {
//
//        abel::internal::ExposedMetricsInTsc ready_to_run_latency(
//                "abel/fiber/latency/ready_to_run");
//        abel::internal::ExposedMetricsInTsc start_fibers_latency(
//                "abel/fiber/latency/start_fibers");
//        abel::internal::ExposedMetricsInTsc wakeup_sleeping_worker_latency(
//                "abel/fiber/latency/wakeup_sleeping_worker");
//        ExposedCounter<std::uint64_t> spinning_worker_wakeups(
//                "abel/fiber/scheduling_group/spinning_worker_wakeups");
//        ExposedCounter<std::uint64_t> sleeping_worker_wakeups(
//                "abel/fiber/scheduling_group/sleeping_worker_wakeups");
//        ExposedCounter<std::uint64_t> no_worker_available(
//                "abel/fiber/scheduling_group/no_worker_available");
//
//// If desired, user can report this timer to their monitoring system.
//        abel::internal::BuiltinMonitoredTimer ready_to_run_latency_monitoring(
//                "fiber_latency_ready_to_run", 1us);

        namespace {
        /*
            std::string WriteBitMask(std::uint64_t x) {
                std::string s(64, 0);
                for (int i = 0; i != 64; ++i) {
                    s[i] = ((x & (1ULL << i)) ? '1' : '0');
                }
                return s;
            }*/

        }  // namespace

        // This class guarantees no wake-up loss by keeping a "wake-up count". If a wake
        // operation is made before a wait, the subsequent wait is immediately
        // satisfied without actual going to sleep.
#if defined(ABEL_PLATFORM_LINUX)
        class alignas(hardware_destructive_interference_size)
                scheduling_group::wait_slot {
        public:
            void wake() noexcept {
//                scoped_deferred _([start = ReadTsc()] {
//                    wakeup_sleeping_worker_latency->Report(TscElapsed(start, ReadTsc()));
//                });

                if (wakeup_count_.fetch_add(1, std::memory_order_relaxed) == 0) {
                    DCHECK(syscall(SYS_futex, &wakeup_count_, FUTEX_WAKE_PRIVATE, 1, 0,
                                         0, 0) >= 0);
                }
                // If `Wait()` is called before this check fires, `wakeup_count_` can be 0.
                DCHECK_GE(wakeup_count_.load(std::memory_order_relaxed), 0);
            }

            void wait() noexcept {
                if (wakeup_count_.fetch_sub(1, std::memory_order_relaxed) == 1) {
                    do {
                        // TODO(yinbinli): I saw spurious wake up. But how can it happen? If
                        // `wakeup_count_` is not zero by the time `futex` checks it, the only
                        // values it can become is a positive one, which in this case is a
                        // "real" wake up.
                        //
                        // We need further investigation here.
                        auto rc =
                                syscall(SYS_futex, &wakeup_count_, FUTEX_WAIT_PRIVATE, 0, 0, 0, 0);
                        DCHECK(rc == 0 || errno == EAGAIN);
                    } while (wakeup_count_.load(std::memory_order_relaxed) == 0);
                }
                DCHECK_GT(wakeup_count_.load(std::memory_order_relaxed), 0);
            }

            void persistent_wake() noexcept {
                // Hopefully this is large enough.
                wakeup_count_.store(0x4000'0000, std::memory_order_relaxed);
                DCHECK(syscall(SYS_futex, &wakeup_count_, FUTEX_WAKE_PRIVATE, INT_MAX,
                                     0, 0, 0) >= 0);
            }

        private:
            // `futex` requires this.
            static_assert(sizeof(std::atomic<int>) == sizeof(int));

            std::atomic<int> wakeup_count_{1};
        };
#else

        class alignas(hardware_destructive_interference_size)
                scheduling_group::wait_slot {
        public:
            void wake() noexcept {
//                scoped_deferred _([start = ReadTsc()] {
//                    wakeup_sleeping_worker_latency->Report(TscElapsed(start, ReadTsc()));
//                });
                std::unique_lock<std::mutex> lk(mutex_);
                // If `Wait()` is called before this check fires, `wakeup_count_` can be 0.
                ++wakeup_count_;
                cond_.notify_one();
            }

            void wait() noexcept {
                std::unique_lock<std::mutex> lk(mutex_);
                if (wakeup_count_ != kPersist && wakeup_count_  <= 1) {
                    while (wakeup_count_ <= 1) {
                        if (cond_.wait_for(lk, std::chrono::milliseconds(10)) != std::cv_status::timeout
                            || wakeup_count_ == kPersist) {
                            break;
                        }
                    }
                }
                if(wakeup_count_ != kPersist) {
                    wakeup_count_ -= 1;
                }
            }

            void persistent_wake() noexcept {
                std::unique_lock<std::mutex> lk(mutex_);
                //DLOG_INFO("wake all");
                wakeup_count_ = kPersist;
                cond_.notify_all();
            }

        private:
            // `futex` requires this.
            static constexpr int kPersist = -40000;
            std::mutex mutex_;
            std::condition_variable cond_;
            int wakeup_count_{1};
        };

#endif

        ABEL_INTERNAL_TLS_MODEL thread_local std::size_t
                scheduling_group::worker_index_ = kUninitializedWorkerIndex;

        scheduling_group::scheduling_group(const core_affinity &affinity,
                                           std::size_t size)
                : group_size_(size),
                  affinity_(affinity),
                  run_queue_(FLAGS_fiber_run_queue_size) {
            // We use bitmask (a `std::uint64_t`) to save workers' state. That puts an
            // upper limit on how many workers can be in a given scheduling group.
            DCHECK_MSG(group_size_ <= 64,
                       "groups: {}, We only support up to 64 workers in each scheduling group. "
                       "Use more scheduling groups if you want more concurrency.", group_size_);

            // Exposes states about this scheduling group.
//            auto exposed_var_prefix =
//                    Format("abel/fiber/scheduling_group/{}/", fmt::ptr(this));
//
//            spinning_workers_var_.Init(exposed_var_prefix + "spinning_workers", [this] {
//                return WriteBitMask(spinning_workers_.load(std::memory_order_relaxed));
//            });
//            sleeping_workers_var_.Init(exposed_var_prefix + "sleeping_workers", [this] {
//                return WriteBitMask(sleeping_workers_.load(std::memory_order_relaxed));
//            });

            wait_slots_ = std::make_unique<wait_slot[]>(group_size_);
        }

        scheduling_group::~scheduling_group() = default;

        fiber_entity *scheduling_group::acquire_fiber() noexcept {
            if (auto rc = run_queue_.pop()) {
                // Acquiring the lock here guarantees us anyone who is working on this fiber
                // (with the lock held) has done its job before we returning it to the
                // caller (worker).
                std::scoped_lock _(rc->scheduler_lock);

                DCHECK(rc->state == fiber_state::Ready);
                rc->state = fiber_state::Running;

//                auto now = ReadTsc();
//                ready_to_run_latency->Report(TscElapsed(rc->last_ready_tsc, now));
//                ready_to_run_latency_monitoring.Report(
//                        DurationFromTsc(rc->last_ready_tsc, now));
                return rc;
            }
            return stopped_.load(std::memory_order_relaxed) ? kSchedulingGroupShuttingDown
                                                            : nullptr;
        }

        fiber_entity *scheduling_group::spinning_acquire_fiber() noexcept {
            // We don't want too many workers spinning, it wastes CPU cycles.
            static constexpr auto kMaximumSpinners = 2;

            DCHECK_NE(worker_index_, kUninitializedWorkerIndex);
            DCHECK_LT(worker_index_, group_size_);

            fiber_entity *fiber = nullptr;
            auto spinning = spinning_workers_.load(std::memory_order_relaxed);
            auto mask = 1ULL << worker_index_;
            bool need_spin = false;

            // Simply test `spinning` and try to spin may result in too many workers to
            // spin, as it there's a time windows between we test `spinning` and set our
            // bit in it.
            while (count_non_zeros(spinning) < kMaximumSpinners) {
                DCHECK_EQ(spinning & mask, 0);
                if (spinning_workers_.compare_exchange_weak(spinning, spinning | mask,
                                                            std::memory_order_relaxed)) {
                    need_spin = true;
                    break;
                }
            }

            if (need_spin) {
                static constexpr auto kMaximumCyclesToSpin = abel::duration::nanoseconds(10000);
                // Wait for some time between touching `run_queue_` to reduce contention.
                static constexpr auto kCyclesBetweenRetry = abel::duration::nanoseconds(1000);
                auto start = abel::time_now(), end = start + kMaximumCyclesToSpin;

                scoped_deferred _([&] {
                    // Note that we can actually clear nothing, the same bit can be cleared by
                    // `WakeOneSpinningWorker` simultaneously. This is okay though, as we'll
                    // try `acquire_fiber()` when we leave anyway.
                    spinning_workers_.fetch_and(~mask, std::memory_order_relaxed);
                });

                do {
                    if (auto rc = acquire_fiber()) {
                        fiber = rc;
                        break;
                    }
                    auto next = start + kCyclesBetweenRetry;
                    while (start < next) {
                        if (pending_spinner_wakeup_.load(std::memory_order_relaxed) &&
                            pending_spinner_wakeup_.exchange(false,
                                                             std::memory_order_relaxed)) {
                            // There's a pending wakeup, and it's us who is chosen to finish this
                            // job.
                            wake_up_one_deep_sleeping_worker();
                        } else {
                            pause<16>();
                        }
                        start = abel::time_now();
                    }
                } while (start < end &&
                         (spinning_workers_.load(std::memory_order_relaxed) & mask));
            } else {
                // Otherwise there are already at least 2 workers spinning, don't waste CPU
                // cycles then.
                return nullptr;
            }

            if (fiber || ((fiber = acquire_fiber()))) {
                // Given that we successfully grabbed a fiber to run, we're likely under
                // load. So wake another worker to spin (if there are not enough spinners).
                //
                // We don't want to wake it here, though, as we've had something useful to
                // do (run `fiber)`, so we leave it for other spinners to wake as they have
                // nothing useful to do anyway.
                if (count_non_zeros(spinning_workers_.load(std::memory_order_relaxed)) <
                    kMaximumSpinners) {
                    pending_spinner_wakeup_.store(true, std::memory_order_relaxed);
                }
            }
            return fiber;
        }

        fiber_entity *scheduling_group::wait_for_fiber() noexcept {
            DCHECK_NE(worker_index_, kUninitializedWorkerIndex);
            DCHECK_LT(worker_index_, group_size_);
            auto mask = 1ULL << worker_index_;

            while (true) {
                scoped_deferred _([&] {
                    // If we're woken up before we even sleep (i.e., we're "woken up" after we
                    // set the bit in `sleeping_workers_` but before we actually called
                    // `wait_slot::Wait()`), this effectively clears nothing.
                    sleeping_workers_.fetch_and(~mask, std::memory_order_relaxed);
                });
                DCHECK_EQ(
                        sleeping_workers_.fetch_or(mask, std::memory_order_relaxed) & mask, 0);

                // We should test if the queue is indeed empty, otherwise if a new fiber is
                // put into the ready queue concurrently, and whoever makes the fiber ready
                // checked the sleeping mask before we updated it, we'll lose the fiber.
                if (auto f = acquire_fiber()) {
                    // A new fiber is put into ready queue concurrently then.
                    //
                    // If our sleeping mask has already been cleared (by someone else), we
                    // need to wake up yet another sleeping worker (otherwise it's a wakeup
                    // miss.).
                    //
                    // Note that in this case the "deferred" clean up is not needed actually.
                    // This is a rare case, though. TODO(yinbinli): Optimize it away.
                    if ((sleeping_workers_.fetch_and(~mask, std::memory_order_relaxed) &
                         mask) == 0) {
                        // Someone woken us up before we cleared the flag, wake up a new worker
                        // for him.
                        wake_up_one_worker();
                    }
                    return f;
                }

                wait_slots_[worker_index_].wait();

                // We only return non-`nullptr` here. If we return `nullptr` to the caller,
                // it'll go spinning immediately. Doing that will likely waste CPU cycles.
                if (auto f = acquire_fiber()) {
                    return f;
                }  // Otherwise try again (and possibly sleep) until a fiber is ready.
            }
        }

        fiber_entity *scheduling_group::remote_acquire_fiber() noexcept {
            if (auto rc = run_queue_.steal()) {
                std::scoped_lock _(rc->scheduler_lock);

                DCHECK(rc->state == fiber_state::Ready);
                rc->state = fiber_state::Running;
                //ready_to_run_latency->Report(TscElapsed(rc->last_ready_tsc, ReadTsc()));

                // It now belongs to the caller's scheduling group.
                rc->own_scheduling_group = current();
                return rc;
            }
            return nullptr;
        }

        void scheduling_group::start_fibers(fiber_entity **start,
                                            fiber_entity **end) noexcept {
            if (ABEL_UNLIKELY(start == end)) {
                return;  // Why would you call this method then?
            }

            auto tsc = abel::time_now();
//            scoped_deferred _(
//                    [&] { start_fibers_latency->Report(TscElapsed(tsc, ReadTsc())); });

            for (auto iter = start; iter != end; ++iter) {
                (*iter)->state = fiber_state::Ready;
                (*iter)->own_scheduling_group = this;
                (*iter)->last_ready_tsc = tsc;
            }
            if (ABEL_UNLIKELY(!run_queue_.batch_push(start, end, false))) {
                //auto since = abel::time_now();

                while (!run_queue_.batch_push(start, end, false)) {
//                    ABEL_LOG_WARNING_EVERY_SECOND(
//                            "Run queue overflow. Too many ready fibers to run. If you're still "
//                            "not overloaded, consider increasing `fiber_run_queue_size`.");
//                    ABEL_LOG_FATAL_IF(ReadSteadyClock() - since > 5s,
//                                       "Failed to push fiber into ready queue after retrying "
//                                       "for 5s. Gave up.");
                    std::this_thread::sleep_for(100us);
                }
            }
            // TODO(yinbinli): Increment `no_worker_available` accordingly.
            wake_up_workers(end - start);
        }

        void scheduling_group::ready_fiber(
                fiber_entity *fiber, std::unique_lock<abel::fiber_internal::spinlock> &&scheduler_lock) noexcept {
            DCHECK_MSG(!stopped_.load(std::memory_order_relaxed),
                       "The scheduling group has been stopped.");
            DCHECK_MSG(fiber != get_master_fiber_entity(),
                       "Master fiber should not be added to run queue.");

            fiber->state = fiber_state::Ready;
            fiber->own_scheduling_group = this;
            fiber->last_ready_tsc = abel::time_now();
            if (scheduler_lock) {
                scheduler_lock.unlock();
            }

            // push the fiber into run queue and (optionally) wake up a worker.
            if (ABEL_UNLIKELY(!run_queue_.push(fiber, fiber->scheduling_group_local))) {
                DLOG_INFO("push fail");
                auto since = abel::time_now();

                while (!run_queue_.push(fiber, fiber->scheduling_group_local)) {
                    DLOG_EVERY_N_WARN(1000,
                                      "Run queue overflow. Too many ready fibers to run. If you're still "
                                      "not overloaded, consider increasing `fiber_run_queue_size`.");
                    DLOG_IF_CRITICAL(abel::time_now() - since > abel::duration::seconds(5),
                                     "Failed to push fiber into ready queue after retrying "
                                     "for 5s. Gave up.");
                    std::this_thread::sleep_for(100us);
                }
            }
            if (ABEL_UNLIKELY(!wake_up_one_worker())) {
                //no_worker_available->Increment();
                //DLOG_WARN("no avaiable workers");
            }
        }

        void scheduling_group::halt(
                fiber_entity *self, std::unique_lock<abel::fiber_internal::spinlock> &&scheduler_lock) noexcept {
            DCHECK_MSG(self == get_current_fiber_entity(),
                       "`self` must be pointer to caller's `fiber_entity`.");
            DCHECK_MSG(
                    scheduler_lock.owns_lock(),
                    "Scheduler lock must be held by caller prior to call to this method.");
            DCHECK_MSG(
                    self->state == fiber_state::Running,
                    "`halt()` is only for running fiber's use. If you want to `ready_fiber()` "
                    "yourself and `halt()`, what you really need is `yield()`.");
            auto master = get_master_fiber_entity();
            self->state = fiber_state::Waiting;

            // We simply yield to master fiber for now.
            //
            // TODO(yinbinli): We can directly yield to next ready fiber. This way we can
            // eliminate a context switch.
            //
            // Note that we need to hold `scheduler_lock` until we finished context swap.
            // Otherwise if we're in ready queue, we can be resume again even before we
            // stopped running. This will be disastrous.
            //
            // Do NOT pass `scheduler_lock` ('s pointer)` to cb. Instead, play with raw
            // lock.
            //
            // The reason is that, `std::unique_lock<...>::unlock` itself is not an atomic
            // operation (although `Spinlock` is).
            //
            // What this means is that, after unlocking the scheduler lock, and the fiber
            // starts to run again, `std::unique_lock<...>::owns_lock` does not
            // necessarily be updated in time (before the fiber checks it), which can lead
            // to subtle bugs.
            master->resume_on(
                    [self_lock = scheduler_lock.release()]() { self_lock->unlock(); });

            // When we're back, we should be in the same fiber.
            DCHECK_EQ(self, get_current_fiber_entity());
        }

        void scheduling_group::yield(fiber_entity *self) noexcept {
            // TODO(yinbinli): We can directly yield to next ready fiber. This way we can
            // eliminate a context switch.
            auto master = get_master_fiber_entity();

            // Master's state is not maintained in a coherency way..
            master->state = fiber_state::Ready;
            switch_to(self, master);
        }

        void scheduling_group::switch_to(fiber_entity *self, fiber_entity *to) noexcept {
            DCHECK_EQ(self, get_current_fiber_entity());

            // We need scheduler lock here actually (at least to comfort TSan). But so
            // long as this check does not fiber, we're safe without the lock I think.
            DCHECK_MSG(to->state == fiber_state::Ready,
                       "Fiber `to` is not in ready state.");
            DCHECK_MSG(self != to, "Switch to yourself results in U.B.");

            // TODO(yinbinli): Ensure neither `self->scheduler_lock` nor
            // `to->scheduler_lock` is currrently held (by someone else).

            // We delay queuing `self` to run queue until `to` starts to run.
            //
            // It's possible that we first add `self` to run queue with its scheduler lock
            // locked, and unlock the lock when `to` runs. However, if `self` is grabbed
            // by some worker prior `to` starts to run, the worker will spin to wait for
            // `to`. This can be quite costly.
            to->resume_on([this, self]() {
                ready_fiber(self, std::unique_lock(self->scheduler_lock));
            });

            // When we're back, we should be in the same fiber.
            DCHECK_EQ(self, get_current_fiber_entity());
        }

        void scheduling_group::enter_group(std::size_t index) {
            DCHECK_MSG(current_ == nullptr,
                       "This pthread worker has already joined a scheduling group.");
            DCHECK_MSG(timer_worker_ != nullptr,
                       "The timer worker is not available yet.");

            // Initialize TLSes as much as possible. Initializing them need an adequate
            // amount of stack space, and may not be done on system fiber.
            memory_internal::InitializeObjectPoolForCurrentThread();

            // Initialize thread-local timer queue for this worker.
            timer_worker_->initialize_local_queue(index);

            // Initialize scheduling-group information of this pthread.
            current_ = this;
            worker_index_ = index;

            // Initialize master fiber for this worker.
            set_up_master_fiber_entity();
        }

        void scheduling_group::leave_group() {
            DCHECK_MSG(current_ == this,
                       "This pthread worker does not belong to this scheduling group.");
            current_ = nullptr;
            worker_index_ = kUninitializedWorkerIndex;
        }

        std::size_t scheduling_group::group_size() const noexcept { return group_size_; }

        core_affinity scheduling_group::affinity() {
            return affinity_;
        }

        void scheduling_group::set_timer_worker(timer_worker *worker) noexcept {
            timer_worker_ = worker;
        }

        void scheduling_group::stop() {
            stopped_.store(true, std::memory_order_relaxed);
            for (std::size_t index = 0; index != group_size_; ++index) {
                wait_slots_[index].persistent_wake();
            }
        }

        bool scheduling_group::wake_up_one_worker() noexcept {
            return wake_up_one_spinning_worker() || wake_up_one_deep_sleeping_worker();
        }

        bool scheduling_group::wake_up_one_spinning_worker() noexcept {
            // FIXME: Is "relaxed" order sufficient here?
            //DLOG_INFO("wake_up_one_spinning_worker");
            while (auto spinning_mask =
                    spinning_workers_.load(std::memory_order_relaxed)) {
                // Last fiber worker (LSB in `spinning_mask`) that is spinning.
                auto last_spinning = __builtin_ffsll(spinning_mask) - 1;
                auto claiming_mask = 1ULL << last_spinning;
                if (ABEL_LIKELY(spinning_workers_.fetch_and(~claiming_mask,
                                                            std::memory_order_relaxed) &
                                claiming_mask)) {
                    // We cleared the `last_spinning` bit, no one else will try to dispatch
                    // work to it.
                    //spinning_worker_wakeups->Add(1);
                    return true;  // Fast path then.
                }
                pause();
            }  // Keep trying until no one else is spinning.
            return false;
        }

        bool scheduling_group::wake_up_workers(std::size_t n) noexcept {
            DLOG_INFO("wake_up_workers {}", n);
            if (ABEL_UNLIKELY(n == 0)) {
                return false;  // No worker is waken up.
            }
            if (ABEL_UNLIKELY(n == 1)) {
                return wake_up_one_worker();
            }

            // As there are at most two spinners, and `n` is at least two, we can safely
            // claim all spinning workers.
            auto spinning_mask_was =
                    spinning_workers_.exchange(0, std::memory_order_relaxed);
            auto woke = count_non_zeros(spinning_mask_was);
            //sleeping_worker_wakeups->Add(woke);
            DCHECK_LE(static_cast<size_t>(woke), n);
            n -= woke;

            if (n >= group_size_) {
                // If there are more fibers than the group size, wake up all workers.
                auto sleeping_mask_was =
                        sleeping_workers_.exchange(0, std::memory_order_relaxed);
                for (size_t i = 0; i != group_size_; ++i) {
                    if (sleeping_mask_was & (1 << i)) {
                        wait_slots_[i].wake();
                    }
                    // sleeping_worker_wakeups->Add(1);
                }
                return true;
            } else if (n >= 1) {
                while (auto sleeping_mask_was =
                        sleeping_workers_.load(std::memory_order_relaxed)) {
                    auto mask_to = sleeping_mask_was;
                    // Wake up workers with lowest indices.
                    if (static_cast<size_t>(count_non_zeros(sleeping_mask_was)) <= n) {
                        mask_to = 0;  // All workers will be woken up.
                    } else {
                        while (n--) {
                            mask_to &= ~(1 << __builtin_ffsll(mask_to));
                        }
                    }

                    // Try to claim the workers.
                    if (ABEL_LIKELY(sleeping_workers_.compare_exchange_weak(
                            sleeping_mask_was, mask_to, std::memory_order_relaxed))) {
                        auto masked = sleeping_mask_was & ~mask_to;
                        for (size_t i = 0; i != group_size_; ++i) {
                            if (masked & (1 << i)) {
                                wait_slots_[i].wake();
                            }
                            //  sleeping_worker_wakeups->Add(1);
                        }
                        return true;
                    }
                    pause();
                }
            } else {
                return true;
            }
            return false;
        }

        bool scheduling_group::wake_up_one_deep_sleeping_worker() noexcept {
            // We indeed have to wake someone that is in deep sleep then.
            //DLOG_INFO("wake_up_one_deep_sleeping_worker");
            while (auto sleeping_mask =
                    sleeping_workers_.load(std::memory_order_relaxed)) {
                // We always give a preference to workers with a lower index (LSB in
                // `sleeping_mask`).
                //
                // If we're under light load, this way we hopefully can avoid wake workers
                // with higher index at all.
                auto last_sleeping = __builtin_ffsll(sleeping_mask) - 1;
                auto claiming_mask = 1ULL << last_sleeping;
                if (ABEL_LIKELY(sleeping_workers_.fetch_and(~claiming_mask,
                                                            std::memory_order_relaxed) &
                                claiming_mask)) {
                    // We claimed the worker. Let's wake it up then.
                    //
                    // `wait_slot` class it self guaranteed no wake-up loss. So don't worry
                    // about that.
                    DCHECK_LT(static_cast<size_t>(last_sleeping), group_size_);
                    wait_slots_[last_sleeping].wake();
                    //sleeping_worker_wakeups->Add(1);
                    return true;
                }
                pause();
            }
            return false;
        }
    }  // namespace fiber_internal
}  // namespace abel