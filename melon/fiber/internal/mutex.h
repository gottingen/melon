
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef  MELON_FIBER_INTERNAL_MUTEX_H_
#define  MELON_FIBER_INTERNAL_MUTEX_H_

#include "melon/fiber/internal/types.h"
#include "melon/base/scoped_lock.h"
#include "melon/metrics/utils/lock_timer.h"

__BEGIN_DECLS
extern int fiber_mutex_init(fiber_mutex_t *__restrict mutex,
                            const fiber_mutexattr_t *__restrict mutex_attr);
extern int fiber_mutex_destroy(fiber_mutex_t *mutex);
extern int fiber_mutex_trylock(fiber_mutex_t *mutex);
extern int fiber_mutex_lock(fiber_mutex_t *mutex);
extern int fiber_mutex_timedlock(fiber_mutex_t *__restrict mutex,
                                 const struct timespec *__restrict abstime);
extern int fiber_mutex_unlock(fiber_mutex_t *mutex);
__END_DECLS

namespace melon::fiber_internal {

    namespace internal {
        class fast_pthread_mutex {
        public:
            fast_pthread_mutex() : _futex(0) {}

            ~fast_pthread_mutex() {}

            void lock();

            void unlock();

            bool try_lock();

        private:
            MELON_DISALLOW_COPY_AND_ASSIGN(fast_pthread_mutex);

            int lock_contended();

            unsigned _futex;
        };
    }

}  // namespace melon::fiber_internal

// Specialize std::lock_guard and std::unique_lock for fiber_mutex_t

namespace std {

    template<>
    class lock_guard<fiber_mutex_t> {
    public:
        explicit lock_guard(fiber_mutex_t &mutex) : _pmutex(&mutex) {
#if !defined(NDEBUG)
            const int rc = fiber_mutex_lock(_pmutex);
            if (rc) {
                MELON_LOG(FATAL) << "Fail to lock fiber_mutex_t=" << _pmutex << ", " << melon_error(rc);
                _pmutex = NULL;
            }
#else
            fiber_mutex_lock(_pmutex);
#endif  // NDEBUG
        }

        ~lock_guard() {
#ifndef NDEBUG
            if (_pmutex) {
                fiber_mutex_unlock(_pmutex);
            }
#else
            fiber_mutex_unlock(_pmutex);
#endif
        }

    private:
        MELON_DISALLOW_COPY_AND_ASSIGN(lock_guard);

        fiber_mutex_t *_pmutex;
    };

    template<>
    class unique_lock<fiber_mutex_t> {
        MELON_DISALLOW_COPY_AND_ASSIGN(unique_lock);

    public:
        typedef fiber_mutex_t mutex_type;

        unique_lock() : _mutex(NULL), _owns_lock(false) {}

        explicit unique_lock(mutex_type &mutex)
                : _mutex(&mutex), _owns_lock(false) {
            lock();
        }

        unique_lock(mutex_type &mutex, defer_lock_t)
                : _mutex(&mutex), _owns_lock(false) {}

        unique_lock(mutex_type &mutex, try_to_lock_t)
                : _mutex(&mutex), _owns_lock(fiber_mutex_trylock(&mutex) == 0) {}

        unique_lock(mutex_type &mutex, adopt_lock_t)
                : _mutex(&mutex), _owns_lock(true) {}

        ~unique_lock() {
            if (_owns_lock) {
                unlock();
            }
        }

        void lock() {
            if (!_mutex) {
                MELON_CHECK(false) << "Invalid operation";
                return;
            }
            if (_owns_lock) {
                MELON_CHECK(false) << "Detected deadlock issue";
                return;
            }
            fiber_mutex_lock(_mutex);
            _owns_lock = true;
        }

        bool try_lock() {
            if (!_mutex) {
                MELON_CHECK(false) << "Invalid operation";
                return false;
            }
            if (_owns_lock) {
                MELON_CHECK(false) << "Detected deadlock issue";
                return false;
            }
            _owns_lock = !fiber_mutex_trylock(_mutex);
            return _owns_lock;
        }

        void unlock() {
            if (!_owns_lock) {
                MELON_CHECK(false) << "Invalid operation";
                return;
            }
            if (_mutex) {
                fiber_mutex_unlock(_mutex);
                _owns_lock = false;
            }
        }

        void swap(unique_lock &rhs) {
            std::swap(_mutex, rhs._mutex);
            std::swap(_owns_lock, rhs._owns_lock);
        }

        mutex_type *release() {
            mutex_type *saved_mutex = _mutex;
            _mutex = NULL;
            _owns_lock = false;
            return saved_mutex;
        }

        mutex_type *mutex() { return _mutex; }

        bool owns_lock() const { return _owns_lock; }

        operator bool() const { return owns_lock(); }

    private:
        mutex_type *_mutex;
        bool _owns_lock;
    };

}  // namespace std

namespace melon {

    template<>
    struct MutexConstructor<fiber_mutex_t> {
        bool operator()(fiber_mutex_t *mutex) const {
            return fiber_mutex_init(mutex, NULL) == 0;
        }
    };

    template<>
    struct MutexDestructor<fiber_mutex_t> {
        bool operator()(fiber_mutex_t *mutex) const {
            return fiber_mutex_destroy(mutex) == 0;
        }
    };

}  // namespace melon

#endif  // MELON_FIBER_INTERNAL_MUTEX_H_
