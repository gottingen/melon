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


// Date: Mon Sep 22 22:23:13 CST 2014

#ifndef MUTIL_DOUBLY_BUFFERED_DATA_H
#define MUTIL_DOUBLY_BUFFERED_DATA_H

#include <deque>
#include <vector>                                       // std::vector
#include <pthread.h>
#include <melon/utility/scoped_lock.h>
#include <melon/utility/thread_local.h>
#include <turbo/log/logging.h>
#include <melon/utility/macros.h>
#include <melon/utility/type_traits.h>
#include <melon/utility/errno.h>
#include <melon/utility/atomicops.h>
#include <melon/utility/unique_ptr.h>
#include <melon/utility/type_traits.h>

namespace mutil {

// This data structure makes Read() almost lock-free by making Modify()
// *much* slower. It's very suitable for implementing LoadBalancers which
// have a lot of concurrent read-only ops from many threads and occasional
// modifications of data. As a side effect, this data structure can store
// a thread-local data for user.
//
// --- `AllowFiberSuspended=false' ---
// Read(): Begin with a thread-local mutex locked then read the foreground
// instance which will not be changed before the mutex is unlocked. Since the
// mutex is only locked by Modify() with an empty critical section, the
// function is almost lock-free.
//
// Modify(): Modify background instance which is not used by any Read(), flip
// foreground and background, lock thread-local mutexes one by one to make
// sure all existing Read() finish and later Read() see new foreground,
// then modify background(foreground before flip) again.
//
// But, when `AllowFiberSuspended=false', it is not allowed to suspend fiber
// while reading. Otherwise, it may cause deadlock.
//
//
// --- `AllowFiberSuspended=true' ---
// It is allowed to suspend fiber while reading.
// It is not allowed to use non-Void TLS.
// If fiber will not be suspended while reading, it also makes Read() almost
// lock-free by making Modify() *much* slower.
// If fiber will be suspended while reading, there is competition among
// fibers using the same Wrapper.
//
// Read(): Begin with thread-local reference count of foreground instance
// incremented by one which be protected by a thread-local mutex, then read
// the foreground instance which will not be changed before its all thread-local
// reference count become zero. At last, after the query completes, thread-local
// reference count of foreground instance will be decremented by one, and if
// it becomes zero, notifies Modify().
//
// Modify(): Modify background instance which is not used by any Read(), flip
// foreground and background, lock thread-local mutexes one by one and wait
// until thread-local reference counts which be protected by a thread-local
// mutex become 0 to make sure all existing Read() finish and later Read()
// see new foreground, then modify background(foreground before flip) again.

class Void { };

template <typename T> struct IsVoid : false_type { };
template <> struct IsVoid<Void> : true_type { };

template <typename T, typename TLS = Void, bool AllowFiberSuspended = false>
class DoublyBufferedData {
    class Wrapper;
    class WrapperTLSGroup;
    typedef int WrapperTLSId;
public:
    class ScopedPtr {
    friend class DoublyBufferedData;
    public:
        ScopedPtr() : _data(NULL), _index(0), _w(NULL) {}
        ~ScopedPtr() {
            if (_w) {
                if (AllowFiberSuspended) {
                    _w->EndRead(_index);
                } else {
                    _w->EndRead();
                }
            }
        }
        const T* get() const { return _data; }
        const T& operator*() const { return *_data; }
        const T* operator->() const { return _data; }
        TLS& tls() { return _w->user_tls(); }
        
    private:
        DISALLOW_COPY_AND_ASSIGN(ScopedPtr);
        const T* _data;
        // Index of foreground instance used by ScopedPtr.
        int _index;
        Wrapper* _w;
    };
    
    DoublyBufferedData();
    ~DoublyBufferedData();

    // Put foreground instance into ptr. The instance will not be changed until
    // ptr is destructed.
    // This function is not blocked by Read() and Modify() in other threads.
    // Returns 0 on success, -1 otherwise.
    int Read(ScopedPtr* ptr);

    // Modify background and foreground instances. fn(T&, ...) will be called
    // twice. Modify() from different threads are exclusive from each other.
    // NOTE: Call same series of fn to different equivalent instances should
    // result in equivalent instances, otherwise foreground and background
    // instance will be inconsistent.
    template <typename Fn> size_t Modify(Fn& fn);
    template <typename Fn, typename Arg1> size_t Modify(Fn& fn, const Arg1&);
    template <typename Fn, typename Arg1, typename Arg2>
    size_t Modify(Fn& fn, const Arg1&, const Arg2&);

    // fn(T& background, const T& foreground, ...) will be called to background
    // and foreground instances respectively.
    template <typename Fn> size_t ModifyWithForeground(Fn& fn);
    template <typename Fn, typename Arg1>
    size_t ModifyWithForeground(Fn& fn, const Arg1&);
    template <typename Fn, typename Arg1, typename Arg2>
    size_t ModifyWithForeground(Fn& fn, const Arg1&, const Arg2&);
    
private:
    template <typename Fn>
    struct WithFG0 {
        WithFG0(Fn& fn, T* data) : _fn(fn), _data(data) { }
        size_t operator()(T& bg) {
            return _fn(bg, (const T&)_data[&bg == _data]);
        }
    private:
        Fn& _fn;
        T* _data;
    };

    template <typename Fn, typename Arg1>
    struct WithFG1 {
        WithFG1(Fn& fn, T* data, const Arg1& arg1)
            : _fn(fn), _data(data), _arg1(arg1) {}
        size_t operator()(T& bg) {
            return _fn(bg, (const T&)_data[&bg == _data], _arg1);
        }
    private:
        Fn& _fn;
        T* _data;
        const Arg1& _arg1;
    };

    template <typename Fn, typename Arg1, typename Arg2>
    struct WithFG2 {
        WithFG2(Fn& fn, T* data, const Arg1& arg1, const Arg2& arg2)
            : _fn(fn), _data(data), _arg1(arg1), _arg2(arg2) {}
        size_t operator()(T& bg) {
            return _fn(bg, (const T&)_data[&bg == _data], _arg1, _arg2);
        }
    private:
        Fn& _fn;
        T* _data;
        const Arg1& _arg1;
        const Arg2& _arg2;
    };

    template <typename Fn, typename Arg1>
    struct Closure1 {
        Closure1(Fn& fn, const Arg1& arg1) : _fn(fn), _arg1(arg1) {}
        size_t operator()(T& bg) { return _fn(bg, _arg1); }
    private:
        Fn& _fn;
        const Arg1& _arg1;
    };

    template <typename Fn, typename Arg1, typename Arg2>
    struct Closure2 {
        Closure2(Fn& fn, const Arg1& arg1, const Arg2& arg2)
            : _fn(fn), _arg1(arg1), _arg2(arg2) {}
        size_t operator()(T& bg) { return _fn(bg, _arg1, _arg2); }
    private:
        Fn& _fn;
        const Arg1& _arg1;
        const Arg2& _arg2;
    };

    const T* UnsafeRead() const {
        return _data + _index.load(mutil::memory_order_acquire);
    }

    const T* UnsafeRead(int& index) const {
        index = _index.load(mutil::memory_order_acquire);
        return _data + index;
    }

    Wrapper* AddWrapper(Wrapper*);
    void RemoveWrapper(Wrapper*);

    // Foreground and background void.
    T _data[2];

    // Index of foreground instance.
    mutil::atomic<int> _index;

    // Key to access thread-local wrappers.
    WrapperTLSId _wrapper_key;

    // All thread-local instances.
    std::vector<Wrapper*> _wrappers;

    // Sequence access to _wrappers.
    pthread_mutex_t _wrappers_mutex{};

    // Sequence modifications.
    pthread_mutex_t _modify_mutex{};
};

static const pthread_key_t INVALID_PTHREAD_KEY = (pthread_key_t)-1;

template <typename T, typename TLS>
class DoublyBufferedDataWrapperBase {
public:
    TLS& user_tls() { return _user_tls; }
protected:
    TLS _user_tls;
};

template <typename T>
class DoublyBufferedDataWrapperBase<T, Void> {
};

// Use pthread_key store data limits by _SC_THREAD_KEYS_MAX.
// WrapperTLSGroup can store Wrapper in thread local storage.
// WrapperTLSGroup will destruct Wrapper data when thread exits,
// other times only reset Wrapper inner structure.
template <typename T, typename TLS, bool AllowFiberSuspended>
class DoublyBufferedData<T, TLS, AllowFiberSuspended>::WrapperTLSGroup {
public:
    const static size_t RAW_BLOCK_SIZE = 4096;
    const static size_t ELEMENTS_PER_BLOCK = (RAW_BLOCK_SIZE + sizeof(T) - 1) / sizeof(T);

    struct MELON_CACHELINE_ALIGNMENT ThreadBlock {
        inline DoublyBufferedData::Wrapper* at(size_t offset) {
            return _data + offset;
        };

    private:
        DoublyBufferedData::Wrapper _data[ELEMENTS_PER_BLOCK];
    };

    inline static WrapperTLSId key_create() {
        MELON_SCOPED_LOCK(_s_mutex);
        WrapperTLSId id = 0;
        if (!_get_free_ids().empty()) {
            id = _get_free_ids().back();
            _get_free_ids().pop_back();
        } else {
            id = _s_id++;
        }
        return id;
    }

    inline static int key_delete(WrapperTLSId id) {
        MELON_SCOPED_LOCK(_s_mutex);
        if (id < 0 || id >= _s_id) {
            errno = EINVAL;
            return -1;
        }
        _get_free_ids().push_back(id);
        return 0;
    }

    inline static DoublyBufferedData::Wrapper* get_or_create_tls_data(WrapperTLSId id) {
        if (MELON_UNLIKELY(id < 0)) {
            CHECK(false) << "Invalid id=" << id;
            return NULL;
        }
        if (_s_tls_blocks == NULL) {
            _s_tls_blocks = new (std::nothrow) std::vector<ThreadBlock*>;
            if (MELON_UNLIKELY(_s_tls_blocks == NULL)) {
                LOG(FATAL) << "Fail to create vector, " << berror();
                return NULL;
            }
            mutil::thread_atexit(_destroy_tls_blocks);
        }
        const size_t block_id = (size_t)id / ELEMENTS_PER_BLOCK;
        if (block_id >= _s_tls_blocks->size()) {
            // The 32ul avoid pointless small resizes.
            _s_tls_blocks->resize(std::max(block_id + 1, 32ul));
        }
        ThreadBlock* tb = (*_s_tls_blocks)[block_id];
        if (tb == NULL) {
            ThreadBlock* new_block = new (std::nothrow) ThreadBlock;
            if (MELON_UNLIKELY(new_block == NULL)) {
                return NULL;
            }
            tb = new_block;
            (*_s_tls_blocks)[block_id] = new_block;
        }
        return tb->at(id - block_id * ELEMENTS_PER_BLOCK);
    }

private:
    static void _destroy_tls_blocks() {
        if (!_s_tls_blocks) {
            return;
        }
        for (size_t i = 0; i < _s_tls_blocks->size(); ++i) {
            delete (*_s_tls_blocks)[i];
        }
        delete _s_tls_blocks;
        _s_tls_blocks = NULL;
    }

    inline static std::deque<WrapperTLSId>& _get_free_ids() {
        if (MELON_UNLIKELY(!_s_free_ids)) {
            _s_free_ids = new (std::nothrow) std::deque<WrapperTLSId>();
            RELEASE_ASSERT(_s_free_ids);
        }
        return *_s_free_ids;
    }

private:
    static pthread_mutex_t _s_mutex;
    static WrapperTLSId _s_id;
    static std::deque<WrapperTLSId>* _s_free_ids;
    static __thread std::vector<ThreadBlock*>* _s_tls_blocks;
};

template <typename T, typename TLS, bool AllowFiberSuspended>
pthread_mutex_t DoublyBufferedData<T, TLS, AllowFiberSuspended>::WrapperTLSGroup::_s_mutex = PTHREAD_MUTEX_INITIALIZER;

template <typename T, typename TLS, bool AllowFiberSuspended>
std::deque<typename DoublyBufferedData<T, TLS, AllowFiberSuspended>::WrapperTLSId>*
        DoublyBufferedData<T, TLS, AllowFiberSuspended>::WrapperTLSGroup::_s_free_ids = NULL;

template <typename T, typename TLS, bool AllowFiberSuspended>
typename DoublyBufferedData<T, TLS, AllowFiberSuspended>::WrapperTLSId
        DoublyBufferedData<T, TLS, AllowFiberSuspended>::WrapperTLSGroup::_s_id = 0;

template <typename T, typename TLS, bool AllowFiberSuspended>
__thread std::vector<typename DoublyBufferedData<T, TLS, AllowFiberSuspended>::WrapperTLSGroup::ThreadBlock*>*
        DoublyBufferedData<T, TLS, AllowFiberSuspended>::WrapperTLSGroup::_s_tls_blocks = NULL;

template <typename T, typename TLS, bool AllowFiberSuspended>
class DoublyBufferedData<T, TLS, AllowFiberSuspended>::Wrapper
    : public DoublyBufferedDataWrapperBase<T, TLS> {
friend class DoublyBufferedData;
public:
    explicit Wrapper()
        : _control(NULL)
        , _modify_wait(false) {
        pthread_mutex_init(&_mutex, NULL);
        if (AllowFiberSuspended) {
            pthread_cond_init(&_cond[0], NULL);
            pthread_cond_init(&_cond[1], NULL);
        }
    }
    
    ~Wrapper() {
        if (_control != NULL) {
            _control->RemoveWrapper(this);
        }

        if (AllowFiberSuspended) {
            WaitReadDone(0);
            WaitReadDone(1);

            pthread_cond_destroy(&_cond[0]);
            pthread_cond_destroy(&_cond[1]);
        }

        pthread_mutex_destroy(&_mutex);
    }

    // _mutex will be locked by the calling pthread and DoublyBufferedData.
    // Most of the time, no modifications are done, so the mutex is
    // uncontended and fast.
    inline void BeginRead() {
        pthread_mutex_lock(&_mutex);
    }

    // For `AllowFiberSuspended=true'.
    inline void BeginReadRelease() {
        pthread_mutex_unlock(&_mutex);
    }

    inline void EndRead() {
        pthread_mutex_unlock(&_mutex);
    }

    // For `AllowFiberSuspended=true'.
    // Thread-local reference count which be protected by _mutex
    // will be decremented by one.
    inline void EndRead(int index) {
        MELON_SCOPED_LOCK(_mutex);
        SubRef(index);
        SignalReadCond(index);
    }

    inline void WaitReadDone() {
        MELON_SCOPED_LOCK(_mutex);
    }

    // For `AllowFiberSuspended=true'.
    // Wait until all read of foreground instance done.
    inline void WaitReadDone(int index) {
        MELON_SCOPED_LOCK(_mutex);
        int& ref = index == 0 ? _ref[0] : _ref[1];
        while (ref != 0) {
            _modify_wait = true;
            pthread_cond_wait(&_cond[index], &_mutex);
        }
        _modify_wait = false;
    }

    // For `AllowFiberSuspended=true'.
    inline void SignalReadCond(int index) {
        if (_ref[index] == 0 && _modify_wait) {
            pthread_cond_signal(&_cond[index]);
        }
    }

    // For `AllowFiberSuspended=true'.
    void AddRef(int index) {
        ++_ref[index];
    }

    // For `AllowFiberSuspended=true'.
    void SubRef(int index) {
        --_ref[index];
    }
    
private:
    DoublyBufferedData* _control;
    pthread_mutex_t _mutex{};
    // For `AllowFiberSuspended=true'.
    // _cond[0] for _ref[0], _cond[1] for _ref[1]
    pthread_cond_t _cond[2]{};
    // For `AllowFiberSuspended=true'.
    // _ref[0] is reference count for _data[0],
    // _ref[1] is reference count for _data[1].
    int _ref[2]{0, 0};
    // For `AllowFiberSuspended=true'.
    // Whether there is a Modify() waiting for _ref0/_ref1.
    bool _modify_wait;
};

// Called when thread initializes thread-local wrapper.
template <typename T, typename TLS, bool AllowFiberSuspended>
typename DoublyBufferedData<T, TLS, AllowFiberSuspended>::Wrapper*
DoublyBufferedData<T, TLS, AllowFiberSuspended>::AddWrapper(
        typename DoublyBufferedData<T, TLS, AllowFiberSuspended>::Wrapper* w) {
    if (NULL == w) {
        return NULL;
    }
    if (w->_control == this) {
        return w;
    }
    if (w->_control != NULL) {
        LOG(FATAL) << "Get wrapper from tls but control != this";
        return NULL;
    }
    try {
        w->_control = this;
        MELON_SCOPED_LOCK(_wrappers_mutex);
        _wrappers.push_back(w);
    } catch (std::exception& e) {
        return NULL;
    }
    return w;
}

// Called when thread quits.
template <typename T, typename TLS, bool AllowFiberSuspended>
void DoublyBufferedData<T, TLS, AllowFiberSuspended>::RemoveWrapper(
    typename DoublyBufferedData<T, TLS, AllowFiberSuspended>::Wrapper* w) {
    if (NULL == w) {
        return;
    }
    MELON_SCOPED_LOCK(_wrappers_mutex);
    for (size_t i = 0; i < _wrappers.size(); ++i) {
        if (_wrappers[i] == w) {
            _wrappers[i] = _wrappers.back();
            _wrappers.pop_back();
            return;
        }
    }
}

template <typename T, typename TLS, bool AllowFiberSuspended>
DoublyBufferedData<T, TLS, AllowFiberSuspended>::DoublyBufferedData()
    : _index(0)
    , _wrapper_key(0) {
    MELON_CASSERT(!(AllowFiberSuspended && !IsVoid<TLS>::value),
                  "Forbidden to allow fiber suspended with non-Void TLS");

    _wrappers.reserve(64);
    pthread_mutex_init(&_modify_mutex, NULL);
    pthread_mutex_init(&_wrappers_mutex, NULL);
    _wrapper_key = WrapperTLSGroup::key_create();
    // Initialize _data for some POD types. This is essential for pointer
    // types because they should be Read() as NULL before any Modify().
    if (is_integral<T>::value || is_floating_point<T>::value ||
        is_pointer<T>::value || is_member_function_pointer<T>::value) {
        _data[0] = T();
        _data[1] = T();
    }
}

template <typename T, typename TLS, bool AllowFiberSuspended>
DoublyBufferedData<T, TLS, AllowFiberSuspended>::~DoublyBufferedData() {
    // User is responsible for synchronizations between Read()/Modify() and
    // this function.
    
    {
        MELON_SCOPED_LOCK(_wrappers_mutex);
        for (size_t i = 0; i < _wrappers.size(); ++i) {
            _wrappers[i]->_control = NULL;  // hack: disable removal.
        }
        _wrappers.clear();
    }
    WrapperTLSGroup::key_delete(_wrapper_key);
    _wrapper_key = -1;
    pthread_mutex_destroy(&_modify_mutex);
    pthread_mutex_destroy(&_wrappers_mutex);
}

template <typename T, typename TLS, bool AllowFiberSuspended>
int DoublyBufferedData<T, TLS, AllowFiberSuspended>::Read(
    typename DoublyBufferedData<T, TLS, AllowFiberSuspended>::ScopedPtr* ptr) {
    Wrapper* p = WrapperTLSGroup::get_or_create_tls_data(_wrapper_key);
    Wrapper* w = AddWrapper(p);
    if (MELON_LIKELY(w != NULL)) {
        if (AllowFiberSuspended) {
            // Use reference count instead of mutex to indicate read of
            // foreground instance, so during the read process, there is
            // no need to lock mutex and fiber is allowed to be suspended.
            w->BeginRead();
            int index = -1;
            ptr->_data = UnsafeRead(index);
            ptr->_index = index;
            w->AddRef(index);
            ptr->_w = w;
            w->BeginReadRelease();
        } else {
            w->BeginRead();
            ptr->_data = UnsafeRead();
            ptr->_w = w;
        }

        return 0;
    }
    return -1;
}

template <typename T, typename TLS, bool AllowFiberSuspended>
template <typename Fn>
size_t DoublyBufferedData<T, TLS, AllowFiberSuspended>::Modify(Fn& fn) {
    // _modify_mutex sequences modifications. Using a separate mutex rather
    // than _wrappers_mutex is to avoid blocking threads calling
    // AddWrapper() or RemoveWrapper() too long. Most of the time, modifications
    // are done by one thread, contention should be negligible.
    MELON_SCOPED_LOCK(_modify_mutex);
    int bg_index = !_index.load(mutil::memory_order_relaxed);
    // background instance is not accessed by other threads, being safe to
    // modify.
    const size_t ret = fn(_data[bg_index]);
    if (!ret) {
        return 0;
    }

    // Publish, flip background and foreground.
    // The release fence matches with the acquire fence in UnsafeRead() to
    // make readers which just begin to read the new foreground instance see
    // all changes made in fn.
    _index.store(bg_index, mutil::memory_order_release);
    bg_index = !bg_index;
    
    // Wait until all threads finishes current reading. When they begin next
    // read, they should see updated _index.
    {
        MELON_SCOPED_LOCK(_wrappers_mutex);
        for (size_t i = 0; i < _wrappers.size(); ++i) {
            // Wait read of old foreground instance done.
            if (AllowFiberSuspended) {
                _wrappers[i]->WaitReadDone(bg_index);
            } else {
                _wrappers[i]->WaitReadDone();
            }
        }
    }

    const size_t ret2 = fn(_data[bg_index]);
    CHECK_EQ(ret2, ret) << "index=" << _index.load(mutil::memory_order_relaxed);
    return ret2;
}

template <typename T, typename TLS, bool AllowFiberSuspended>
template <typename Fn, typename Arg1>
size_t DoublyBufferedData<T, TLS, AllowFiberSuspended>::Modify(Fn& fn, const Arg1& arg1) {
    Closure1<Fn, Arg1> c(fn, arg1);
    return Modify(c);
}

template <typename T, typename TLS, bool AllowFiberSuspended>
template <typename Fn, typename Arg1, typename Arg2>
size_t DoublyBufferedData<T, TLS, AllowFiberSuspended>::Modify(
    Fn& fn, const Arg1& arg1, const Arg2& arg2) {
    Closure2<Fn, Arg1, Arg2> c(fn, arg1, arg2);
    return Modify(c);
}

template <typename T, typename TLS, bool AllowFiberSuspended>
template <typename Fn>
size_t DoublyBufferedData<T, TLS, AllowFiberSuspended>::ModifyWithForeground(Fn& fn) {
    WithFG0<Fn> c(fn, _data);
    return Modify(c);
}

template <typename T, typename TLS, bool AllowFiberSuspended>
template <typename Fn, typename Arg1>
size_t DoublyBufferedData<T, TLS, AllowFiberSuspended>::ModifyWithForeground(Fn& fn, const Arg1& arg1) {
    WithFG1<Fn, Arg1> c(fn, _data, arg1);
    return Modify(c);
}

template <typename T, typename TLS, bool AllowFiberSuspended>
template <typename Fn, typename Arg1, typename Arg2>
size_t DoublyBufferedData<T, TLS, AllowFiberSuspended>::ModifyWithForeground(
    Fn& fn, const Arg1& arg1, const Arg2& arg2) {
    WithFG2<Fn, Arg1, Arg2> c(fn, _data, arg1, arg2);
    return Modify(c);
}

}  // namespace mutil

#endif  // MUTIL_DOUBLY_BUFFERED_DATA_H
