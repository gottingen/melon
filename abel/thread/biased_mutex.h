//
// Created by liyinbin on 2021/4/5.
//

#ifndef ABEL_THREAD_BIASED_MUTEX_H_
#define ABEL_THREAD_BIASED_MUTEX_H_


#include <atomic>
#include <mutex>

#include "abel/base/annotation.h"
#include "abel/thread/internal/barrier.h"
#include "abel/base/profile.h"

namespace abel {

    namespace thread_internal {

        // Helper class for implementing fast-path of `biased_mutex.`
        template<class Owner>
        class blessed_side {
        public:
            void lock();

            void unlock();

        private:
            void lock_slow();

            Owner *get_owner() noexcept { return static_cast<Owner *>(this); }
        };

        // Helper class for implementing slow-path of `biased_mutex.`
        template<class Owner>
        class really_slow_side {
        public:
            void lock();

            void unlock();

        private:
            Owner *get_owner() noexcept { return static_cast<Owner *>(this); }
        };

    }  // namespace thread_internal

    // TL;DR: DO NOT USE IT. IT'S TERRIBLY SLOW.
    //
    // This mutex is "biased" in that it boosts one ("blessed") side's perf. in
    // grabbing lock, by sacrificing other contenders. This mutex can boost overall
    // perf. if you're using it in scenarios where you have separate fast-path and
    // slow-path (which should be executed rare). Note that there can only be one
    // "blessed" side. THE SLOW SIDE IS **REALLY REALLY** SLOW AND MAY HAVE A
    // NEGATIVE IMPACT ON OTHER THREADS (esp. considering that the heavy side of our
    // asymmetric memory barrier performs really bad). Incorrect use of this mutex
    // can hurt performane badly. YOU HAVE BEEN WARNED.
    //
    // Note that it's a SPINLOCK. In case your critical section is long, do not use
    // it.
    //
    // Internally it's a "Dekker's lock". By using asymmetric memory barrier (@sa:
    // `memory_barrier.h`), we can eliminate both RMW atomic & "actual" memory
    // barrier in fast path.
    //
    // @sa: https://en.wikipedia.org/wiki/Dekker%27s_algorithm
    //
    // Usage:
    //
    // // Fast path.
    // std::scoped_lock _(*biased_mutex.blessed_side());
    //
    // // Slow path.
    // std::scoped_lock _(*biased_mutex.really_slow_side());
    class biased_mutex : private thread_internal::blessed_side<biased_mutex>,
                         private thread_internal::really_slow_side<biased_mutex> {
        using blessed_side = thread_internal::blessed_side<biased_mutex>;
        using really_slow_side = thread_internal::really_slow_side<biased_mutex>;

        friend class thread_internal::blessed_side<biased_mutex>;

        friend class thread_internal::really_slow_side<biased_mutex>;

    public:
#ifdef ABEL_INTERNAL_USE_TSAN
        biased_mutex() {
    ABEL_INTERNAL_TSAN_MUTEX_CREATE(this, __tsan_mutex_not_static);
  }

  ~biased_mutex() { ABEL_INTERNAL_TSAN_MUTEX_DESTROY(this, 0); }
#endif

        blessed_side *get_blessed_side() { return this; }

        really_slow_side *get_really_slow_side() { return this; }

    private:
        std::atomic<bool> wants_to_enter_[2] = {};
        std::atomic<std::uint8_t> turn_ = 0;

        // (Our implementation of) Dekker's lock only allows two participants, so we
        // use this lock to serialize contenders in slow path.
        std::mutex slow_lock_lock_;
    };


    namespace thread_internal {

        template<class Owner>
        inline void blessed_side<Owner>::lock() {
            ABEL_INTERNAL_TSAN_MUTEX_PRE_LOCK(this, 0);

            get_owner()->wants_to_enter_[0].store(true, std::memory_order_relaxed);
            asymmetric_barrier_light();
            // There's no need to synchronizes with "other" bless-side -- There won't be
            // one. This lock permits only one bless-side, i.e., us.
            //
            // Therefore we only have to synchronize with the slow side. This is achieved
            // by acquiring on `wants_to_enter_[1]`.
            if (ABEL_UNLIKELY(
                    get_owner()->wants_to_enter_[1].load(std::memory_order_acquire))) {
                lock_slow();
            }

            ABEL_INTERNAL_TSAN_MUTEX_POST_LOCK(this, 0, 0);
        }

        template<class Owner>
        [[gnu::noinline]] void blessed_side<Owner>::lock_slow() {
            asymmetric_barrier_light();  // Not necessary, TBH.
            while (ABEL_UNLIKELY(
                    // Synchronizes with the slow side.
                    get_owner()->wants_to_enter_[1].load(std::memory_order_acquire))) {
                if (get_owner()->turn_.load(std::memory_order_relaxed) != 0) {
                    get_owner()->wants_to_enter_[0].store(false, std::memory_order_relaxed);
                    while (get_owner()->turn_.load(std::memory_order_relaxed) != 0) {
                        // Spin.
                    }
                    get_owner()->wants_to_enter_[0].store(true, std::memory_order_relaxed);
                    asymmetric_barrier_light();
                }
            }
        }

        template<class Owner>
        inline void blessed_side<Owner>::unlock() {
            ABEL_INTERNAL_TSAN_MUTEX_PRE_UNLOCK(this, 0);

            get_owner()->turn_.store(1, std::memory_order_relaxed);
            // Synchronizes with the slow side.
            get_owner()->wants_to_enter_[0].store(false, std::memory_order_release);

            ABEL_INTERNAL_TSAN_MUTEX_POST_UNLOCK(this, 0);
        }

        template<class Owner>
        void really_slow_side<Owner>::lock() {
            ABEL_INTERNAL_TSAN_MUTEX_PRE_LOCK(this, 0);

            get_owner()->slow_lock_lock_.lock();
            get_owner()->wants_to_enter_[1].store(true, std::memory_order_relaxed);
            asymmetric_barrier_heavy();
            while (ABEL_UNLIKELY(
                    // Synchronizes with the fast side.
                    get_owner()->wants_to_enter_[0].load(std::memory_order_acquire))) {
                if (get_owner()->turn_.load(std::memory_order_relaxed) != 1) {
                    get_owner()->wants_to_enter_[1].store(false, std::memory_order_relaxed);
                    while (get_owner()->turn_.load(std::memory_order_relaxed) != 1) {
                        // Spin.
                    }
                    get_owner()->wants_to_enter_[1].store(true, std::memory_order_relaxed);
                    asymmetric_barrier_heavy();
                }
            }

            ABEL_INTERNAL_TSAN_MUTEX_POST_LOCK(this, 0, 0);
        }

        template<class Owner>
        void really_slow_side<Owner>::unlock() {
            ABEL_INTERNAL_TSAN_MUTEX_PRE_UNLOCK(this, 0);

            get_owner()->turn_.store(0, std::memory_order_relaxed);
            // Synchronizes with the fast side.
            get_owner()->wants_to_enter_[1].store(false, std::memory_order_release);
            get_owner()->slow_lock_lock_.unlock();

            ABEL_INTERNAL_TSAN_MUTEX_POST_UNLOCK(this, 0);
        }

    }  // namespace thread_internal

}  // namespace abel

#endif  // ABEL_THREAD_BIASED_MUTEX_H_
