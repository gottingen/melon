
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef MELON_THREAD_RW_LOCK_H_
#define MELON_THREAD_RW_LOCK_H_

#include <pthread.h>
#include <stdint.h>
#include <mutex>
#include "melon/base/profile.h"

namespace melon {

    enum lock_mode {
        INVALID_LOCK, READ_LOCK, WRITE_LOCK
    };

    class rw_lock {
    public:
        rw_lock() { pthread_rwlock_init(&_lock, nullptr); }

        bool lock(lock_mode mode) {
            switch (mode) {
                case READ_LOCK: {
                    return 0 == pthread_rwlock_rdlock(&_lock);
                }
                case WRITE_LOCK: {
                    return 0 == pthread_rwlock_wrlock(&_lock);
                }
                default: {
                    return false;
                }
            }
        }

        bool unlock(lock_mode mode) {
            MELON_UNUSED(mode);
            return 0 == pthread_rwlock_unlock(&_lock);
        }

        ~rw_lock() { pthread_rwlock_destroy(&_lock); }

    private:
        pthread_rwlock_t _lock;
    };

    // read only
    class read_lock {
    public:
        explicit read_lock(rw_lock &lock) : _lock(lock) {}

        void lock() { _lock.lock(READ_LOCK); }

        void unlock() { _lock.unlock(READ_LOCK); }

    private:
        rw_lock &_lock;
    };

    // write lock
    class write_lock {
    public:
        explicit write_lock(rw_lock &lock) : _lock(lock) {}

        void lock() { _lock.lock(WRITE_LOCK); }

        void unlock() { _lock.unlock(WRITE_LOCK); }

    private:
        rw_lock &_lock;
    };

}  // namespace melon

namespace std {

    template<>
    class lock_guard<melon::read_lock> {
    public:
        explicit lock_guard(melon::read_lock &mutex) : _pmutex(&mutex) {
            _pmutex->lock();
        }

        ~lock_guard() {
            _pmutex->unlock();
        }

    private:
        MELON_DISALLOW_COPY_AND_ASSIGN(lock_guard);

        melon::read_lock *_pmutex;
    };

    template<>
    class lock_guard<melon::write_lock> {
    public:
        explicit lock_guard(melon::write_lock &mutex) : _pmutex(&mutex) {
            _pmutex->lock();
        }

        ~lock_guard() {
            _pmutex->unlock();
        }

    private:
        MELON_DISALLOW_COPY_AND_ASSIGN(lock_guard);

        melon::write_lock *_pmutex;
    };

}  // namespace std

#endif // MELON_THREAD_RW_LOCK_H_
