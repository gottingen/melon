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


#ifndef MUTIL_MELON_SCOPED_LOCK_H
#define MUTIL_MELON_SCOPED_LOCK_H

#include <melon/utility/build_config.h>

#if defined(MUTIL_CXX11_ENABLED)
#include <mutex>                           // std::lock_guard
#endif

#include <melon/utility/synchronization/lock.h>
#include <melon/utility/macros.h>
#include <turbo/log/logging.h>
#include <melon/utility/errno.h>

#if !defined(MUTIL_CXX11_ENABLED)
#define MELON_SCOPED_LOCK(ref_of_lock)                                  \
    std::lock_guard<MELON_TYPEOF(ref_of_lock)>                          \
    MELON_CONCAT(scoped_locker_dummy_at_line_, __LINE__)(ref_of_lock)
#else

// NOTE(gejun): c++11 deduces additional reference to the type.
namespace mutil {
namespace detail {
template <typename T>
std::lock_guard<typename std::remove_reference<T>::type> get_lock_guard();
}  // namespace detail
}  // namespace mutil

#define MELON_SCOPED_LOCK(ref_of_lock)                                  \
    decltype(::mutil::detail::get_lock_guard<decltype(ref_of_lock)>()) \
    MELON_CONCAT(scoped_locker_dummy_at_line_, __LINE__)(ref_of_lock)
#endif

namespace std {

#if !defined(MUTIL_CXX11_ENABLED)

// Do not acquire ownership of the mutex
struct defer_lock_t {};
static const defer_lock_t defer_lock = {};

// Try to acquire ownership of the mutex without blocking 
struct try_to_lock_t {};
static const try_to_lock_t try_to_lock = {};

// Assume the calling thread already has ownership of the mutex 
struct adopt_lock_t {};
static const adopt_lock_t adopt_lock = {};

template <typename Mutex> class lock_guard {
public:
    explicit lock_guard(Mutex & mutex) : _pmutex(&mutex) { _pmutex->lock(); }
    ~lock_guard() { _pmutex->unlock(); }
private:
    DISALLOW_COPY_AND_ASSIGN(lock_guard);
    Mutex* _pmutex;
};

template <typename Mutex> class unique_lock {
    DISALLOW_COPY_AND_ASSIGN(unique_lock);
public:
    typedef Mutex mutex_type;
    unique_lock() : _mutex(NULL), _owns_lock(false) {}
    explicit unique_lock(mutex_type& mutex)
        : _mutex(&mutex), _owns_lock(true) {
        mutex.lock();
    }
    unique_lock(mutex_type& mutex, defer_lock_t)
        : _mutex(&mutex), _owns_lock(false)
    {}
    unique_lock(mutex_type& mutex, try_to_lock_t) 
        : _mutex(&mutex), _owns_lock(mutex.try_lock())
    {}
    unique_lock(mutex_type& mutex, adopt_lock_t) 
        : _mutex(&mutex), _owns_lock(true)
    {}

    ~unique_lock() {
        if (_owns_lock) {
            _mutex->unlock();
        }
    }

    void lock() {
        if (_owns_lock) {
            CHECK(false) << "Detected deadlock issue";
            return;
        }
        _owns_lock = true;
        _mutex->lock();
    }

    bool try_lock() {
        if (_owns_lock) {
            CHECK(false) << "Detected deadlock issue";
            return false;
        }
        _owns_lock = _mutex->try_lock();
        return _owns_lock;
    }

    void unlock() {
        if (!_owns_lock) {
            CHECK(false) << "Invalid operation";
            return;
        }
        _mutex->unlock();
        _owns_lock = false;
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

#endif // !defined(MUTIL_CXX11_ENABLED)

#if defined(OS_POSIX)

template<> class lock_guard<pthread_mutex_t> {
public:
    explicit lock_guard(pthread_mutex_t & mutex) : _pmutex(&mutex) {
#if !defined(NDEBUG)
        const int rc = pthread_mutex_lock(_pmutex);
        if (rc) {
            LOG(FATAL) << "Fail to lock pthread_mutex_t=" << _pmutex << ", " << berror(rc);
            _pmutex = NULL;
        }
#else
        pthread_mutex_lock(_pmutex);
#endif  // NDEBUG
    }
    
    ~lock_guard() {
#ifndef NDEBUG
        if (_pmutex) {
            pthread_mutex_unlock(_pmutex);
        }
#else
        pthread_mutex_unlock(_pmutex);
#endif
    }
    
private:
    DISALLOW_COPY_AND_ASSIGN(lock_guard);
    pthread_mutex_t* _pmutex;
};

template<> class lock_guard<pthread_spinlock_t> {
public:
    explicit lock_guard(pthread_spinlock_t & spin) : _pspin(&spin) {
#if !defined(NDEBUG)
        const int rc = pthread_spin_lock(_pspin);
        if (rc) {
            LOG(FATAL) << "Fail to lock pthread_spinlock_t=" << _pspin << ", " << berror(rc);
            _pspin = NULL;
        }
#else
        pthread_spin_lock(_pspin);
#endif  // NDEBUG
    }
    
    ~lock_guard() {
#ifndef NDEBUG
        if (_pspin) {
            pthread_spin_unlock(_pspin);
        }
#else
        pthread_spin_unlock(_pspin);
#endif
    }
    
private:
    DISALLOW_COPY_AND_ASSIGN(lock_guard);
    pthread_spinlock_t* _pspin;
};

template<> class unique_lock<pthread_mutex_t> {
    DISALLOW_COPY_AND_ASSIGN(unique_lock);
public:
    typedef pthread_mutex_t         mutex_type;
    unique_lock() : _mutex(NULL), _owns_lock(false) {}
    explicit unique_lock(mutex_type& mutex)
        : _mutex(&mutex), _owns_lock(true) {
        pthread_mutex_lock(_mutex);
    }
    unique_lock(mutex_type& mutex, defer_lock_t)
        : _mutex(&mutex), _owns_lock(false)
    {}
    unique_lock(mutex_type& mutex, try_to_lock_t) 
        : _mutex(&mutex), _owns_lock(pthread_mutex_trylock(&mutex) == 0)
    {}
    unique_lock(mutex_type& mutex, adopt_lock_t) 
        : _mutex(&mutex), _owns_lock(true)
    {}

    ~unique_lock() {
        if (_owns_lock) {
            pthread_mutex_unlock(_mutex);
        }
    }

    void lock() {
        if (_owns_lock) {
            CHECK(false) << "Detected deadlock issue";
            return;
        }
#if !defined(NDEBUG)
        const int rc = pthread_mutex_lock(_mutex);
        if (rc) {
            LOG(FATAL) << "Fail to lock pthread_mutex=" << _mutex << ", " << berror(rc);
            return;
        }
        _owns_lock = true;
#else
        _owns_lock = true;
        pthread_mutex_lock(_mutex);
#endif  // NDEBUG
    }

    bool try_lock() {
        if (_owns_lock) {
            CHECK(false) << "Detected deadlock issue";
            return false;
        }
        _owns_lock = !pthread_mutex_trylock(_mutex);
        return _owns_lock;
    }

    void unlock() {
        if (!_owns_lock) {
            CHECK(false) << "Invalid operation";
            return;
        }
        pthread_mutex_unlock(_mutex);
        _owns_lock = false;
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

template<> class unique_lock<pthread_spinlock_t> {
    DISALLOW_COPY_AND_ASSIGN(unique_lock);
public:
    typedef pthread_spinlock_t  mutex_type;
    unique_lock() : _mutex(NULL), _owns_lock(false) {}
    explicit unique_lock(mutex_type& mutex)
        : _mutex(&mutex), _owns_lock(true) {
        pthread_spin_lock(_mutex);
    }

    ~unique_lock() {
        if (_owns_lock) {
            pthread_spin_unlock(_mutex);
        }
    }
    unique_lock(mutex_type& mutex, defer_lock_t)
        : _mutex(&mutex), _owns_lock(false)
    {}
    unique_lock(mutex_type& mutex, try_to_lock_t) 
        : _mutex(&mutex), _owns_lock(pthread_spin_trylock(&mutex) == 0)
    {}
    unique_lock(mutex_type& mutex, adopt_lock_t) 
        : _mutex(&mutex), _owns_lock(true)
    {}

    void lock() {
        if (_owns_lock) {
            CHECK(false) << "Detected deadlock issue";
            return;
        }
#if !defined(NDEBUG)
        const int rc = pthread_spin_lock(_mutex);
        if (rc) {
            LOG(FATAL) << "Fail to lock pthread_spinlock=" << _mutex << ", " << berror(rc);
            return;
        }
        _owns_lock = true;
#else
        _owns_lock = true;
        pthread_spin_lock(_mutex);
#endif  // NDEBUG
    }

    bool try_lock() {
        if (_owns_lock) {
            CHECK(false) << "Detected deadlock issue";
            return false;
        }
        _owns_lock = !pthread_spin_trylock(_mutex);
        return _owns_lock;
    }

    void unlock() {
        if (!_owns_lock) {
            CHECK(false) << "Invalid operation";
            return;
        }
        pthread_spin_unlock(_mutex);
        _owns_lock = false;
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

#endif  // defined(OS_POSIX)

}  // namespace std

namespace mutil {

// Lock both lck1 and lck2 without the dead lock issue
template <typename Mutex1, typename Mutex2>
void double_lock(std::unique_lock<Mutex1> &lck1, std::unique_lock<Mutex2> &lck2) {
    DCHECK(!lck1.owns_lock());
    DCHECK(!lck2.owns_lock());
    volatile void* const ptr1 = lck1.mutex();
    volatile void* const ptr2 = lck2.mutex();
    DCHECK_NE(ptr1, ptr2);
    if (ptr1 < ptr2) {
        lck1.lock();
        lck2.lock();
    } else {
        lck2.lock();
        lck1.lock();
    }
}

};

#endif  // MUTIL_MELON_SCOPED_LOCK_H
