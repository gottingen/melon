//
// Copyright (C) 2024 EA group inc.
// Author: Jeff.li lijippy@163.com
// All rights reserved.
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//
//


#ifndef  MELON_FIBER_MUTEX_H_
#define  MELON_FIBER_MUTEX_H_

#include <melon/fiber/types.h>
#include <melon/utility/scoped_lock.h>
#include <melon/var/utils/lock_timer.h>

__BEGIN_DECLS
extern int fiber_mutex_init(fiber_mutex_t* __restrict mutex,
                              const fiber_mutexattr_t* __restrict mutex_attr);
extern int fiber_mutex_destroy(fiber_mutex_t* mutex);
extern int fiber_mutex_trylock(fiber_mutex_t* mutex);
extern int fiber_mutex_lock(fiber_mutex_t* mutex);
extern int fiber_mutex_timedlock(fiber_mutex_t* __restrict mutex,
                                   const struct timespec* __restrict abstime);
extern int fiber_mutex_unlock(fiber_mutex_t* mutex);
__END_DECLS

namespace fiber {

// The C++ Wrapper of fiber_mutex

// NOTE: Not aligned to cacheline as the container of Mutex is practically aligned
class Mutex {
public:
    typedef fiber_mutex_t* native_handler_type;
    Mutex() {
        int ec = fiber_mutex_init(&_mutex, NULL);
        if (ec != 0) {
            throw std::system_error(std::error_code(ec, std::system_category()), "Mutex constructor failed");
        }
    }
    ~Mutex() { MCHECK_EQ(0, fiber_mutex_destroy(&_mutex)); }
    native_handler_type native_handler() { return &_mutex; }
    void lock() {
        int ec = fiber_mutex_lock(&_mutex);
        if (ec != 0) {
            throw std::system_error(std::error_code(ec, std::system_category()), "Mutex lock failed");
        }
    }
    void unlock() { fiber_mutex_unlock(&_mutex); }
    bool try_lock() { return !fiber_mutex_trylock(&_mutex); }
    // TODO(chenzhangyi01): Complement interfaces for C++11
private:
    DISALLOW_COPY_AND_ASSIGN(Mutex);
    fiber_mutex_t _mutex;
};

namespace internal {
#ifdef FIBER_USE_FAST_PTHREAD_MUTEX
class FastPthreadMutex {
public:
    FastPthreadMutex() : _futex(0) {}
    ~FastPthreadMutex() {}
    void lock();
    void unlock();
    bool try_lock();
private:
    DISALLOW_COPY_AND_ASSIGN(FastPthreadMutex);
    int lock_contended();
    unsigned _futex;
};
#else
typedef mutil::Mutex FastPthreadMutex;
#endif
}

}  // namespace fiber

// Specialize std::lock_guard and std::unique_lock for fiber_mutex_t

namespace std {

template <> class lock_guard<fiber_mutex_t> {
public:
    explicit lock_guard(fiber_mutex_t & mutex) : _pmutex(&mutex) {
#if !defined(NDEBUG)
        const int rc = fiber_mutex_lock(_pmutex);
        if (rc) {
            MLOG(FATAL) << "Fail to lock fiber_mutex_t=" << _pmutex << ", " << berror(rc);
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
    DISALLOW_COPY_AND_ASSIGN(lock_guard);
    fiber_mutex_t* _pmutex;
};

template <> class unique_lock<fiber_mutex_t> {
    DISALLOW_COPY_AND_ASSIGN(unique_lock);
public:
    typedef fiber_mutex_t         mutex_type;
    unique_lock() : _mutex(NULL), _owns_lock(false) {}
    explicit unique_lock(mutex_type& mutex)
        : _mutex(&mutex), _owns_lock(false) {
        lock();
    }
    unique_lock(mutex_type& mutex, defer_lock_t)
        : _mutex(&mutex), _owns_lock(false)
    {}
    unique_lock(mutex_type& mutex, try_to_lock_t) 
        : _mutex(&mutex), _owns_lock(fiber_mutex_trylock(&mutex) == 0)
    {}
    unique_lock(mutex_type& mutex, adopt_lock_t) 
        : _mutex(&mutex), _owns_lock(true)
    {}

    ~unique_lock() {
        if (_owns_lock) {
            unlock();
        }
    }

    void lock() {
        if (!_mutex) {
            MCHECK(false) << "Invalid operation";
            return;
        }
        if (_owns_lock) {
            MCHECK(false) << "Detected deadlock issue";
            return;
        }
        fiber_mutex_lock(_mutex);
        _owns_lock = true;
    }

    bool try_lock() {
        if (!_mutex) {
            MCHECK(false) << "Invalid operation";
            return false;
        }
        if (_owns_lock) {
            MCHECK(false) << "Detected deadlock issue";
            return false;
        }
        _owns_lock = !fiber_mutex_trylock(_mutex);
        return _owns_lock;
    }

    void unlock() {
        if (!_owns_lock) {
            MCHECK(false) << "Invalid operation";
            return;
        }
        if (_mutex) {
            fiber_mutex_unlock(_mutex);
            _owns_lock = false;
        }
    }

    void swap(unique_lock& rhs) {
        std::swap(_mutex, rhs._mutex);
        std::swap(_owns_lock, rhs._owns_lock);
    }

    mutex_type* release() {
        mutex_type* saved_mutex = _mutex;
        _mutex = NULL;
        _owns_lock = false;
        return saved_mutex;
    }

    mutex_type* mutex() { return _mutex; }
    bool owns_lock() const { return _owns_lock; }
    operator bool() const { return owns_lock(); }

private:
    mutex_type*                     _mutex;
    bool                            _owns_lock;
};

}  // namespace std

namespace melon::var {

template <>
struct MutexConstructor<fiber_mutex_t> {
    bool operator()(fiber_mutex_t* mutex) const {
        return fiber_mutex_init(mutex, NULL) == 0;
    }
};

template <>
struct MutexDestructor<fiber_mutex_t> {
    bool operator()(fiber_mutex_t* mutex) const {
        return fiber_mutex_destroy(mutex) == 0;
    }
};

}  // namespace melon::var

#endif  // MELON_FIBER_MUTEX_H_
