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


#ifndef MUTIL_SYNCHRONIZATION_LOCK_H_
#define MUTIL_SYNCHRONIZATION_LOCK_H_

#include <melon/utility/build_config.h>
#if defined(OS_WIN)
#include <windows.h>
#elif defined(OS_POSIX)
#include <pthread.h>
#endif

#include "melon/utility/base_export.h"
#include <melon/utility/macros.h>
#include <melon/utility/compat.h>

namespace mutil {

// A convenient wrapper for an OS specific critical section.  
class MUTIL_EXPORT Mutex {
    DISALLOW_COPY_AND_ASSIGN(Mutex);
public:
#if defined(OS_WIN)
  typedef CRITICAL_SECTION NativeHandle;
#elif defined(OS_POSIX)
  typedef pthread_mutex_t NativeHandle;
#endif

public:
    Mutex() {
#if defined(OS_WIN)
    // The second parameter is the spin count, for short-held locks it avoid the
    // contending thread from going to sleep which helps performance greatly.
        ::InitializeCriticalSectionAndSpinCount(&_native_handle, 2000);
#elif defined(OS_POSIX)
        pthread_mutex_init(&_native_handle, NULL);
#endif
    }
    
    ~Mutex() {
#if defined(OS_WIN)
        ::DeleteCriticalSection(&_native_handle);
#elif defined(OS_POSIX)
        pthread_mutex_destroy(&_native_handle);
#endif
    }

    // Locks the mutex. If another thread has already locked the mutex, a call
    // to lock will block execution until the lock is acquired.
    void lock() {
#if defined(OS_WIN)
        ::EnterCriticalSection(&_native_handle);
#elif defined(OS_POSIX)
        pthread_mutex_lock(&_native_handle);
#endif
    }

    // Unlocks the mutex. The mutex must be locked by the current thread of
    // execution, otherwise, the behavior is undefined.
    void unlock() {
#if defined(OS_WIN)
        ::LeaveCriticalSection(&_native_handle);
#elif defined(OS_POSIX)
        pthread_mutex_unlock(&_native_handle);
#endif
    }
    
    // Tries to lock the mutex. Returns immediately.
    // On successful lock acquisition returns true, otherwise returns false.
    bool try_lock() {
#if defined(OS_WIN)
        return (::TryEnterCriticalSection(&_native_handle) != FALSE);
#elif defined(OS_POSIX)
        return pthread_mutex_trylock(&_native_handle) == 0;
#endif
    }

    // Returns the underlying implementation-defined native handle object.
    NativeHandle* native_handle() { return &_native_handle; }

private:
#if defined(OS_POSIX)
    // The posix implementation of ConditionVariable needs to be able
    // to see our lock and tweak our debugging counters, as it releases
    // and acquires locks inside of pthread_cond_{timed,}wait.
friend class ConditionVariable;
#elif defined(OS_WIN)
// The Windows Vista implementation of ConditionVariable needs the
// native handle of the critical section.
friend class WinVistaCondVar;
#endif

    NativeHandle _native_handle;
};

// TODO: Remove this type.
class MUTIL_EXPORT Lock : public Mutex {
    DISALLOW_COPY_AND_ASSIGN(Lock);
public:
    Lock() {}
    ~Lock() {}
    void Acquire() { lock(); }
    void Release() { unlock(); }
    bool Try() { return try_lock(); }
    void AssertAcquired() const {}
};

// A helper class that acquires the given Lock while the AutoLock is in scope.
class AutoLock {
public:
    struct AlreadyAcquired {};

    explicit AutoLock(Lock& lock) : lock_(lock) {
        lock_.Acquire();
    }

    AutoLock(Lock& lock, const AlreadyAcquired&) : lock_(lock) {
        lock_.AssertAcquired();
    }

    ~AutoLock() {
        lock_.AssertAcquired();
        lock_.Release();
    }

private:
    Lock& lock_;
    DISALLOW_COPY_AND_ASSIGN(AutoLock);
};

// AutoUnlock is a helper that will Release() the |lock| argument in the
// constructor, and re-Acquire() it in the destructor.
class AutoUnlock {
public:
    explicit AutoUnlock(Lock& lock) : lock_(lock) {
        // We require our caller to have the lock.
        lock_.AssertAcquired();
        lock_.Release();
    }

    ~AutoUnlock() {
        lock_.Acquire();
    }

private:
    Lock& lock_;
    DISALLOW_COPY_AND_ASSIGN(AutoUnlock);
};

}  // namespace mutil

#endif  // MUTIL_SYNCHRONIZATION_LOCK_H_
