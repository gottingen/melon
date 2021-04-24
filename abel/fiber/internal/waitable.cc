//
// Created by liyinbin on 2021/4/4.
//


#include "abel/fiber/internal/waitable.h"

#include <array>
#include <utility>
#include <vector>

#include "abel/log/logging.h"
#include "abel/memory/ref_ptr.h"
#include "abel/fiber/internal/fiber_entity.h"
#include "abel/memory/lazy_init.h"
#include "abel/fiber/internal/scheduling_group.h"

namespace abel {
namespace fiber_internal {
    namespace {

        // Utility for waking up a fiber sleeping on a `waitable` asynchronously.
        class async_waker {
        public:
            // Initialize an `async_waker`.
            async_waker(scheduling_group *sg, fiber_entity *self, wait_block *wb)
                    : sg_(sg), self_(self), wb_(wb) {}

            // The destructor does some sanity checks.
            ~async_waker() { DCHECK_MSG(timer_ ==  0, "Have you called `Cleanup()`?"); }

            // Set a timer to awake `self` once `expires_at` is reached.
            void SetTimer(abel::time_point expires_at) {
                wait_cb_ = make_ref_counted<WaitCb>();
                wait_cb_->waiter = self_;

                // This callback wakes us up if we times out.
                auto timer_cb = [wait_cb = wait_cb_ /* ref-counted */, wb = wb_](auto) {
                    std::scoped_lock lk(wait_cb->lock);
                    if (!wait_cb->awake) {  // It's (possibly) timed out.
                        // We're holding the lock, and `wait_cb->awake` has not been set yet, so
                        // `Cleanup()` cannot possibly finished yet. Therefore, we can be sure
                        // `wb` is still alive.
                        if (wb->satisfied.exchange(true, std::memory_order_relaxed)) {
                            // Someone else satisfied the wait earlier.
                            return;
                        }
                        wait_cb->waiter->own_scheduling_group->ready_fiber(
                                wait_cb->waiter, std::unique_lock(wait_cb->waiter->scheduler_lock));
                    }
                };

                // Set the timeout timer.
                timer_ = sg_->create_timer(expires_at, timer_cb);
                sg_->enable_timer(timer_);
            }

            // Prevent the timer set by this class from waking up `self` again.
            void cleanup() {
                // If `timer_cb` has returned, nothing special; if `timer_cb` has never
                // started, nothing special. But if `timer_cb` is running, we need to
                // prevent it from `ready_fiber` us again (when we immediately sleep on
                // another unrelated thing.).
                sg_->remove_timer(std::exchange(timer_, 0));
                {
                    // Here is the trick.
                    //
                    // We're running now, therefore our `wait_block::satisfied` has been set.
                    // Our `timer_cb` will check the flag, and bail out without waking us
                    // again.
                    std::scoped_lock _(wait_cb_->lock);
                    wait_cb_->awake = true;
                }  // `wait_cb_->awake` has been set, so other fields of us won't be touched
                // by `timer_cb`. we're safe to destruct from now on.
            }

        private:
            // Ref counted as it's used both by us and an asynchronous timer.
            struct WaitCb : ref_counted<WaitCb> {
                abel::fiber_internal::spinlock lock;
                fiber_entity *waiter;
                bool awake = false;
            };

            scheduling_group *sg_;
            fiber_entity *self_;
            wait_block *wb_;
            ref_ptr <WaitCb> wait_cb_;
            std::uint64_t timer_ = 0;
        };

    }  // namespace

        // Implementation of `waitable` goes below.

        bool waitable::add_waiter(wait_block *waiter) {
            std::scoped_lock _(lock_);

            DCHECK(waiter->waiter);
            if (persistent_awakened_) {
                return false;
            }
            waiters_.push_back(waiter);
            return true;
        }

        bool waitable::try_remove_waiter(wait_block *waiter) {
            std::scoped_lock _(lock_);
            return waiters_.erase(waiter);
        }

        fiber_entity *waitable::wake_one() {
            std::scoped_lock _(lock_);
            while (true) {
                auto waiter = waiters_.pop_front();
                if (!waiter) {
                    return nullptr;
                }
                // Memory order is guaranteed by `lock_`.
                if (waiter->satisfied.exchange(true, std::memory_order_relaxed)) {
                    continue;  // It's awakened by someone else.
                }
                return waiter->waiter;
            }
        }

        std::vector<fiber_entity *> waitable::SetPersistentAwakened() {
            std::scoped_lock _(lock_);
            persistent_awakened_ = true;

            std::vector<fiber_entity *> wbs;
            while (auto ptr = waiters_.pop_front()) {
                // Same as `wake_one`.
                if (ptr->satisfied.exchange(true, std::memory_order_relaxed)) {
                    continue;
                }
                wbs.push_back(ptr->waiter);
            }
            return wbs;
        }

        void waitable::ResetAwakened() {
            std::scoped_lock _(lock_);
            persistent_awakened_ = false;
        }

        waitable_timer::waitable_timer(abel::time_point expires_at)
                : sg_(scheduling_group::current()),
                // Does it sense if we use object pool here?
                  impl_(make_ref_counted<waitable_ref_counted>()) {
            // We must not set the timer before `impl_` is initialized.
            timer_id_ = sg_->create_timer(
                    expires_at, [ref = impl_](auto) { on_timer_expired(std::move(ref)); });
            sg_->enable_timer(timer_id_);
        }

        waitable_timer::~waitable_timer() { sg_->remove_timer(timer_id_); }

        void waitable_timer::wait() {
            DCHECK(is_fiber_context_present());

            auto current = get_current_fiber_entity();
            // `wait_block.chain` is initialized by `doubly_linked_list_entry` itself.
            wait_block wb = {.waiter = current};
            std::unique_lock lk(current->scheduler_lock);

            if (impl_->add_waiter(&wb)) {
                current->own_scheduling_group->halt(current, std::move(lk));
                // We'll be awakened by `on_timer_expired()`.
            } else {
                // The timer has fired before, return immediately then.
            }
        }

        void waitable_timer::on_timer_expired(ref_ptr <waitable_ref_counted> ref) {
            auto fibers = ref->SetPersistentAwakened();
            for (auto f : fibers) {
                f->own_scheduling_group->ready_fiber(f, std::unique_lock(f->scheduler_lock));
            }
        }


        void fiber_mutex::unlock() {
            DCHECK(is_fiber_context_present());

            if (auto was = count_.fetch_sub(1, std::memory_order_release); was == 1) {
                // Lucky day, no one is waiting on the mutex.
                //
                // Nothing to do.
            } else {
               DCHECK_GT(was, 1);

                // We need this lock so as to see a consistent state between `count_` and
                // `impl_` ('s internal wait queue).
                std::unique_lock splk(slow_path_lock_);
                auto fiber = impl_.wake_one();
                DCHECK(fiber);  // Otherwise `was` must be 1 (as there's no waiter).
                splk.unlock();
                fiber->own_scheduling_group->ready_fiber(
                        fiber, std::unique_lock(fiber->scheduler_lock));
            }
        }

        void fiber_mutex::lock_slow() {
            DCHECK(is_fiber_context_present());

            if (try_lock()) {
                return;  // Your lucky day.
            }

            // It's locked, take the slow path.
            std::unique_lock splk(slow_path_lock_);

            // Tell the owner that we're waiting for the lock.
            if (count_.fetch_add(1, std::memory_order_acquire) == 0) {
                // The owner released the lock before we incremented `count_`.
                //
                // We're still kind of lucky.
                return;
            }

            // Bad luck then. First we add us to the wait chain.
            auto current = get_current_fiber_entity();
            std::unique_lock slk(current->scheduler_lock);
            wait_block wb = {.waiter = current};
            DCHECK(impl_.add_waiter(&wb));  // This can't fail as we never call
            // `SetPersistentAwakened()`.

            // Now the slow path lock can be unlocked.
            //
            // Indeed it's possible that we're awakened even before we call `halt()`,
            // but this issue is already addressed by `scheduler_lock` (which we're
            // holding).
            splk.unlock();

            // Wait until we're woken by `unlock()`.
            //
            // Given that `scheduler_lock` is held by us, anyone else who concurrently
            // tries to wake us up is blocking on it until `halt()` has completed.
            // Hence no race here.
            current->own_scheduling_group->halt(current, std::move(slk));

            // Lock's owner has awakened us up, the lock is in our hand then.
            DCHECK(!impl_.try_remove_waiter(&wb));
            return;
        }

        // Implementation of `fiber_cond` goes below.

        void fiber_cond::wait(std::unique_lock<fiber_mutex> &lock) {
            DCHECK(is_fiber_context_present());
            DCHECK(lock.owns_lock());

            wait_until(lock, abel::time_point::infinite_future());
        }

        bool fiber_cond::wait_until(
                std::unique_lock<fiber_mutex> &lock,
                abel::time_point expires_at) {
            DCHECK(is_fiber_context_present());

            auto current = get_current_fiber_entity();
            auto sg = current->own_scheduling_group;
            bool use_timeout = expires_at.to_chrono_time() != std::chrono::system_clock::time_point::max();
            lazy_init <async_waker> awaker;

            // Add us to the wait queue.
            std::unique_lock slk(current->scheduler_lock);
            wait_block wb = {.waiter = current};
            DCHECK(impl_.add_waiter(&wb));
            if (use_timeout) {  // Set a timeout if needed.
                awaker.init(sg, current, &wb);
                awaker->SetTimer(expires_at);
            }

            // Release user's lock.
            lock.unlock();

            // Block until being waken up by either `notify_xxx` or the timer.
            sg->halt(current, std::move(slk));  // `slk` is released by `halt()`.

            // Try remove us from the wait chain. This operation will fail if we're
            // awakened by `notify_xxx()`.
            auto timeout = impl_.try_remove_waiter(&wb);

            if (awaker) {
                // Stop the timer we've set.
                awaker->cleanup();
            }

            // Grab the lock again and return.
            lock.lock();
            return !timeout;
        }

        void fiber_cond::notify_one() noexcept {
            DCHECK(is_fiber_context_present());

            auto fiber = impl_.wake_one();
            if (!fiber) {
                return;
            }
            fiber->own_scheduling_group->ready_fiber(fiber,
                                                std::unique_lock(fiber->scheduler_lock));
        }

        void fiber_cond::notify_all() noexcept {
            DCHECK(is_fiber_context_present());

            // We cannot keep calling `notify_one` here. If a waiter immediately goes to
            // sleep again after we wake up it, it's possible that we wake it again when
            // we try to drain the wait chain.
            //
            // So we remove all waiters first, and schedule them then.
            std::array<fiber_entity *, 64> fibers_quick;
            std::size_t array_usage = 0;  // TODO(yinbinli): We need a `FixedVector` in
            // general.

            // We don't want to touch this in most cases.
            //
            // Given that `std::vector::vector()` is not allowed to throw, I do believe it
            // won't allocate memory on construction.
            std::vector<fiber_entity *> fibers_slow;

            while (true) {
                auto fiber = impl_.wake_one();
                if (!fiber) {
                    break;
                }
                if (ABEL_LIKELY(array_usage < std::size(fibers_quick))) {
                    fibers_quick[array_usage++] = fiber;
                } else {
                    fibers_slow.push_back(fiber);
                }
            }

            // Schedule the waiters.
            for (std::size_t index = 0; index != array_usage; ++index) {
                auto &&e = fibers_quick[index];
                e->own_scheduling_group->ready_fiber(e, std::unique_lock(e->scheduler_lock));
            }
            for (auto &&e : fibers_slow) {
                e->own_scheduling_group->ready_fiber(e, std::unique_lock(e->scheduler_lock));
            }
        }

// Implementation of `exit_barrier` goes below.

        exit_barrier::exit_barrier() : count_(1) {}

        std::unique_lock<fiber_mutex> exit_barrier::GrabLock() {
            DCHECK(is_fiber_context_present());
            return std::unique_lock(lock_);
        }

        void exit_barrier::unsafe_count_down(std::unique_lock<fiber_mutex> lk) {
            DCHECK(is_fiber_context_present());
            DCHECK(lk.owns_lock() && lk.mutex() == &lock_);

            // tsan reports a data race if we unlock the lock before notifying the
            // waiters. Although I think it's a false positive, keep the lock before
            // notifying doesn't seem to hurt performance much.
            //
            // lk.unlock();
            DCHECK_GT(count_, 0);
            if (!--count_) {
                cv_.notify_all();
            }
        }

        void exit_barrier::wait() {
            DCHECK(is_fiber_context_present());

            std::unique_lock lk(lock_);
            return cv_.wait(lk, [this] { return count_ == 0; });
        }

// Implementation of `wait_event` goes below.

        void wait_event::wait() {
            DCHECK(is_fiber_context_present());

            auto current = get_current_fiber_entity();
            wait_block wb = {.waiter = current};
            std::unique_lock lk(current->scheduler_lock);
            if (impl_.add_waiter(&wb)) {
                current->own_scheduling_group->halt(current, std::move(lk));
            } else {
                // The event is set already, return immediately.
            }
        }

        void wait_event::set() {
            // `is_fiber_context_present()` is not checked. This method is explicitly allowed
            // to be called out of fiber context.
            std::vector<fiber_entity *> fibers = impl_.SetPersistentAwakened();

            // Fiber wake-up must be delayed until we're done with `impl_`, otherwise
            // `impl_` can be destroyed after its emptied but before we touch it again.
            for (auto &&f : fibers) {
                f->own_scheduling_group->ready_fiber(f, std::unique_lock(f->scheduler_lock));
            }
        }

        oneshot_timed_event::oneshot_timed_event(
                abel::time_point expires_at)
                : sg_(scheduling_group::current()), impl_(make_ref_counted<Impl>()) {
            timer_id_ = sg_->create_timer(
                    expires_at, [ref = impl_](auto) { on_timer_expired(std::move(ref)); });
            sg_->enable_timer(timer_id_);
        }

        oneshot_timed_event::~oneshot_timed_event() { sg_->remove_timer(timer_id_); }

        void oneshot_timed_event::wait() { impl_->event.wait(); }

        void oneshot_timed_event::set() { impl_->IdempotentSet(); }

        void oneshot_timed_event::Impl::IdempotentSet() {
            if (!event_set_guard.exchange(true, std::memory_order_relaxed)) {
                event.set();
            }
        }

        void oneshot_timed_event::on_timer_expired(ref_ptr <Impl> ref) {
            ref->IdempotentSet();
        }
    }  // namespace fiber_internal
}  // namespace abel
