//
// Created by liyinbin on 2021/4/4.
//


#include "abel/fiber/internal/timer_worker.h"

#include <algorithm>
#include <chrono>
#include <limits>
#include <utility>
#include <vector>

#include "abel/base/annotation.h"
#include "abel/memory/object_pool.h"
#include "abel/memory/ref_ptr.h"
#include "abel/fiber/internal/spin_lock.h"
#include "abel/fiber/internal/scheduling_group.h"
#include "abel/thread/lazy_task.h"

using namespace std::literals;

namespace abel {

    template<>
    struct pool_traits<abel::fiber_internal::timer_worker::Entry> {
        static constexpr auto kType = pool_type::ThreadLocal;
        static constexpr auto kLowWaterMark = 65536;
        static constexpr auto kHighWaterMark =
                std::numeric_limits<std::size_t>::max();
        static constexpr auto kMaxIdle = abel::duration::seconds(10);
        static constexpr auto kMinimumThreadCacheSize = 4096;
        static constexpr auto kTransferBatchSize = 1024;

        static void OnPut(abel::fiber_internal::timer_worker::Entry *entry);
    };

}  // namespace abel

namespace abel {
    namespace fiber_internal {

        namespace {

            // Load time point from `expected`, but if it's infinite, return a large timeout
            // instead (otherwise overflow can occur in libstdc++, which results in no wait
            // at all.).
            abel::time_point get_sleep_timeout(abel::time_point expected) {
                if (expected == abel::time_point::from_unix_micros(std::numeric_limits<int64_t>::max())) {
                    return abel::time_now() + abel::duration::seconds(10000);  // Randomly chosen.
                } else {
                    return expected;
                }
            }

        }  // namespace

        struct timer_worker::Entry : abel::pool_ref_counted<timer_worker::Entry> {
            abel::fiber_internal::spinlock lock;  // Protects `cb`.
            std::atomic<bool> cancelled = false;
            bool periodic = false;
            timer_worker *owner;
            abel::function<void(std::uint64_t)> cb;
            abel::time_point expires_at;
            abel::duration interval;
        };

        struct timer_worker::ThreadLocalQueue {
            abel::fiber_internal::spinlock lock;  // To be clear, our critical section size indeed isn't stable
            // (as we can incur heap memory allocation inside it).
            // However, we don't expect the lock to contend much, using a
            // `std::mutex` (which incurs a function call to
            // `pthread_mutex_lock`) here is too high a price to pay.
            std::vector<EntryPtr> timers;
            abel::time_point earliest;

            // This seemingly useless destructor comforts TSan. Otherwise a data race will
            // be reported between this queue's destruction and its last read (by
            // `timer_worker`).
            //
            // Admittedly it's a race, but it only happens when worker exits (i.e. program
            // exits), so we don't care about it.
            ~ThreadLocalQueue() { std::scoped_lock _(lock); }
        };

        thread_local bool tls_queue_initialized = false;

        timer_worker::timer_worker(scheduling_group *sg)
        // `+ 1` below for our own worker thread.
                : sg_(sg), latch(sg_->group_size() + 1), producers_(sg_->group_size() + 1) {}

        timer_worker::~timer_worker() = default;

        timer_worker *timer_worker::get_timer_owner(std::uint64_t timer_id) {
            return reinterpret_cast<Entry *>(timer_id)->owner;
        }

        std::uint64_t timer_worker::create_timer(
                abel::time_point expires_at,
                abel::function<void(std::uint64_t)> &&cb) {
            DCHECK(cb, "No callback for the timer?");

            auto ptr = abel::get_ref_counted<Entry>();
            ptr->owner = this;
            ptr->cancelled.store(false, std::memory_order_relaxed);
            ptr->cb = std::move(cb);
            ptr->expires_at = expires_at;
            ptr->periodic = false;

            DCHECK_EQ(ptr->unsafe_ref_count(), 1);
            return reinterpret_cast<std::uint64_t>(ptr.leak());
        }

        std::uint64_t timer_worker::create_timer(
                abel::time_point initial_expires_at,
                abel::duration interval, abel::function<void(std::uint64_t)> &&cb) {
            DCHECK(cb, "No callback for the timer?");
            DCHECK(interval > abel::duration::nanoseconds(0),
                       "`interval` must be greater than 0 for periodic timers.");
            if (ABEL_UNLIKELY(abel::time_now() > initial_expires_at + abel::duration::seconds(10))) {
//                ABEL_LOG_ERROR_ONCE(
//                        "`initial_expires_at` was specified as long ago. Corrected to now.");
                initial_expires_at = abel::time_now();
            }

            auto ptr = abel::get_ref_counted<Entry>();
            ptr->owner = this;
            ptr->cancelled.store(false, std::memory_order_relaxed);
            ptr->cb = std::move(cb);
            ptr->expires_at = initial_expires_at;
            ptr->interval = interval;
            ptr->periodic = true;

            DCHECK_EQ(ptr->unsafe_ref_count(), 1);
            return reinterpret_cast<std::uint64_t>(ptr.leak());
        }

        void timer_worker::enable_timer(std::uint64_t timer_id) {
            // Ref-count is incremented here. We'll be holding the timer internally.
            add_timer(ref_ptr(ref_ptr_v, reinterpret_cast<Entry *>(timer_id)));
        }

        void timer_worker::remove_timer(std::uint64_t timer_id) {
            ref_ptr ptr(adopt_ptr_v, reinterpret_cast<Entry *>(timer_id));
            DCHECK(ptr->owner == this,
                       "The timer you're trying to detach does not belong to this "
                       "scheduling group.");
            abel::function<void(std::uint64_t)> cb;
            {
                std::scoped_lock _(ptr->lock);
                ptr->cancelled.store(true, std::memory_order_relaxed);
                cb = std::move(ptr->cb);
            }
            // `cb` is released out of the (timer's) lock.
            // Ref-count on `timer_id` is implicitly release by destruction of `ptr`.
        }

        void timer_worker::detach_timer(std::uint64_t timer_id) {
            ref_ptr timer(abel::adopt_ptr_v, reinterpret_cast<Entry *>(timer_id));
            DCHECK(timer->owner == this,
                       "The timer you're trying to detach does not belong to this "
                       "scheduling group.");
            // Ref-count on `timer` is released.
        }

        scheduling_group *timer_worker::get_scheduling_group() { return sg_; }

        void timer_worker::initialize_local_queue(std::size_t worker_index) {
            if (worker_index == scheduling_group::kTimerWorkerIndex) {
                worker_index = sg_->group_size();
            }
            DCHECK_LT(worker_index, sg_->group_size() + 1);
            DCHECK(producers_[worker_index] == nullptr,
                       "Someone else has registered itself as worker #{}.",
                       worker_index);
            producers_[worker_index] = get_thread_local_queue();
            tls_queue_initialized = true;
            latch.count_down();
        }

        void timer_worker::start() {
            abel::core_affinity af;
            worker_ = abel::thread(std::move(af), [&] {
                worker_proc();
            });
        }

        void timer_worker::stop() {
            std::scoped_lock _(lock_);
            stopped_.store(true, std::memory_order_relaxed);
            cv_.notify_one();
        }

        void timer_worker::join() { worker_.join(); }

        void timer_worker::worker_proc() {
            sg_->enter_group(scheduling_group::kTimerWorkerIndex);
            wait_for_workers();  // Wait for other workers to come in.

            while (!stopped_.load(std::memory_order_relaxed)) {
                // We need to reset `next_expires_at_` to a large value, otherwise if
                // someone is inserting a timer that fires later than `next_expires_at_`, it
                // won't touch `next_expires_at_` to reflect this. Later one when we reset
                // `next_expires_at_` (in this method, by calling `WakeWorkerIfNeeded`),
                // that timer will be delayed.
                //
                // This can cause some unnecessary wake up of `cv_` (by
                // `WakeWorkerIfNeeded`), but the wake up operation should be infrequently
                // anyway.
                next_expires_at_.store(std::numeric_limits<int64_t>::max(),
                                       std::memory_order_relaxed);

                // Collect thread-local timer queues into our central heap.
                reap_thread_local_queues();

                // And fire those who has expired.
                fire_timers();

                if (!timers_.empty()) {
                    // Do not reset `next_expires_at_` directly here, we need to compare our
                    // earliest timer with thread-local queues (which is handled by this
                    // `WakeWorkerIfNeeded`).
                    wake_worker_if_needed(timers_.top()->expires_at);
                }

                // Now notify the framework that we'll be free for a while (possibly).
                notify_thread_lazy_task();

                // Sleep until next time fires.
                std::unique_lock lk(lock_);
                auto expected = next_expires_at_.load(std::memory_order_relaxed);
                cv_.wait_until(lk, get_sleep_timeout(abel::time_point::from_unix_micros(expected)).to_chrono_time(), [&] {
                    // We need to check `next_expires_at_` to see if it still equals to the
                    // time we're expected to be awakened. If it doesn't, we need to wake up
                    // early since someone else must added an earlier timer (and, as a
                    // consequence, changed `next_expires_at_`.).
                    return next_expires_at_.load(std::memory_order_relaxed) != expected ||
                           stopped_.load(std::memory_order_relaxed);
                });
            }
            sg_->leave_group();
        }

        void timer_worker::add_timer(EntryPtr timer) {
            DCHECK(tls_queue_initialized,
                       "You must initialize your thread-local queue (done as part of "
                       "`scheduling_group::enter_group()` before calling `AddTimer`.");
            DCHECK_EQ(timer->unsafe_ref_count(), 2);  // One is caller, one is us.

            auto &&tls_queue = get_thread_local_queue();
            std::unique_lock lk(tls_queue->lock);  // This is cheap (relatively, I mean).
            auto expires_at = timer->expires_at;
            tls_queue->timers.push_back(std::move(timer));
            if (tls_queue->earliest > expires_at) {
                tls_queue->earliest = expires_at;
                lk.unlock();
                wake_worker_if_needed(expires_at);
            }
        }

        void timer_worker::wait_for_workers() { latch.wait(); }

        void timer_worker::reap_thread_local_queues() {
            for (auto &&p : producers_) {
                std::vector<EntryPtr> t;
                {
                    std::scoped_lock _(p->lock);
                    t.swap(p->timers);
                    p->earliest = abel::time_point::from_unix_micros(std::numeric_limits<int64_t>::max());
                }
                for (auto &&e : t) {
                    if (e->cancelled.load(std::memory_order_relaxed)) {
                        continue;
                    }
                    timers_.push(std::move(e));
                }
            }
        }

        void timer_worker::fire_timers() {
            auto now = abel::time_now();
            while (!timers_.empty()) {
                auto &&e = timers_.top();
                if (e->cancelled.load(std::memory_order_relaxed)) {
                    timers_.pop();
                    continue;
                }
                if (e->expires_at > now) {
                    break;
                }

                // This IS slow, but if you have many timers to actually *fire*, you're in
                // trouble anyway.
                std::unique_lock lk(e->lock);
                auto cb = std::move(e->cb);
                lk.unlock();
                if (cb) {
                    // `timer_id` is, actually, pointer to `Entry`.
                    cb(reinterpret_cast<std::uint64_t>(e.get()));
                }  // The timer is cancel between we were testing `e->cancelled` and
                // grabbing `e->lock`.

                // If it's a periodic timer, add a new pending timer.
                if (e->periodic) {
                    if (cb) {
                        // CAUTION: Do NOT create a new `Entry` otherwise timer ID we returned
                        // in `AddTimer` will be invalidated.
                        auto cp = std::move(e);  // FIXME: This `std::move` has no effect.
                        std::unique_lock cplk(cp->lock);
                        if (!cp->cancelled) {
                            cp->expires_at = cp->expires_at + cp->interval;
                            cp->cb = std::move(cb);  // Move user's callback back.
                            cplk.unlock();
                            timers_.push(std::move(cp));
                        }
                    } else {
                        DCHECK(e->cancelled.load(std::memory_order_relaxed));
                    }
                }
                timers_.pop();
            }
        }

        void timer_worker::wake_worker_if_needed(
                abel::time_point local_expires_at) {
            auto expires_at = local_expires_at.to_unix_micros();
            auto r = next_expires_at_.load(std::memory_order_relaxed);
            while (true) {
                if (r <= expires_at) {  // Nothing to do then.
                    return;
                }
                // This lock is needed, otherwise we might call `notify_one` after
                // `WorkerProc` tested `next_expires_at_` but before it actually goes into
                // sleep, losing our notification.
                //
                // By grabbing this lock, either we call `notify_one` before `WorkerProc`
                // tests `next_expires_at_`, which is safe; or we call `notify_one` after
                // `WorkerProc` slept and successfully deliver the notification, which
                // is, again, safe.
                //
                // Note that we may NOT do `compare_exchange` first (without lock) and then
                // grab the lock, that way it's still possible to loss wake-ups. Need to
                // grab this lock each time we *try* to move `next_expires_at_` is
                // unfortunate, but this branch should be rare nonetheless.
                std::scoped_lock _(lock_);
                if (next_expires_at_.compare_exchange_weak(r, expires_at)) {
                    // We moved `next_expires_at_` earlier, wake up the worker then.
                    cv_.notify_one();
                    return;
                }
                // `next_expires_at_` has changed, retry then.
            }
        }

        inline timer_worker::ThreadLocalQueue *timer_worker::get_thread_local_queue() {
            ABEL_INTERNAL_TLS_MODEL
            thread_local ThreadLocalQueue q;
            return &q;
        }

        bool timer_worker::EntryPtrComp::operator()(const EntryPtr &p1,
                                                    const EntryPtr &p2) const {
            // `std::priority_queue` orders elements in descending order.
            return p1->expires_at > p2->expires_at;
        }
    }  // namespace fiber_internal
}  // namespace abel

namespace abel {

    void pool_traits<abel::fiber_internal::timer_worker::Entry>::OnPut(
            abel::fiber_internal::timer_worker::Entry *entry) {
        // Free any resources hold by user's callback.
        entry->cb = nullptr;
    }

}  // namespace abel
