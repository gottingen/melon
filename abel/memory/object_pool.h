//
// Created by liyinbin on 2021/4/3.
//

#ifndef ABEL_MEMORY_OBJECT_POOL_H_
#define ABEL_MEMORY_OBJECT_POOL_H_

#include <chrono>
#include <deque>
#include <limits>
#include <memory>
#include <mutex>
#include <utility>

#include "abel/log/logging.h"
#include "abel/base/profile.h"
#include "abel/memory/internal/thread_local.h"
#include "abel/memory/internal/disabled.h"
#include "abel/memory/internal/global.h"
#include "abel/memory/internal/type_descriptor.h"

namespace abel {


    // For the moment, only `MemoryNodeShared` is highly optimized, and it likely
    // will outperform all other type of pools.
    enum class pool_type {
        // Do not use object pool at all.
        //
        // This type is normally used for debugging purpose. (Object pooling makes it
        // hard to tracing object creation, by disabling it, debugging can be easier.)
        Disabled,

        // Cache objects in a thread local cache.
        //
        // This type has the highest performance if your object allocation /
        // deallocation is done evenly in every thread.
        //
        // No lock / synchronization is required for this type of pool
        ThreadLocal,

        // Cache a small amount of objects locally, and use a shared pool for threads
        // in the same NUMA Node.
        //
        // If your objects is allocated in one thread, but freed in other threads in
        // the same scheduling group. This type of pool might work better.
        MemoryNodeShared,

        // Cache a small amount of objects locally, and the rest are cached in a
        // global pool.
        //
        // This type of pool might work not-as-good as the above ones, but if your
        // workload has no evident allocation / deallocation pattern, this type might
        // suit most.
        Global
    };

    // Note that this pool uses thread-local cache. That is, it does not perform
    // well in scenarios such as producer-consumer (in this case, the producer
    // thread keeps allocating objects while the consumer thread keeps de-allocating
    // objects, and nothing could be reused by either thread.). Be aware of this.

    // You need to customize these parameters before using this object pool.
    template<class T>
    struct pool_traits {
        // Type of backend pool to be used for this type. Check comments in `pool_type`
        // for their explanation.
        //
        // static constexpr pool_type kType = ...;  // Or `kpool_type`?

        // If your type cannot be created by `new T()`, you can provide a factory
        // here.
        //
        // Leave it undefined if you don't need it.
        //
        // static T* Create() { ... }

        // If you type cannot be destroyed by `delete ptr`, you can provide a
        // customized deleter here.
        //
        // Leave it undefined if you don't need it.
        //
        // void Destroy(T* ptr) { ... }

        // Below are several hooks.
        //
        // For those hooks you don't need, leave them as not defined.

        // Hook for `get`. It's called after an object is retrieved from the pool.
        // This hook can be used for resetting objects to a "clean" state so that
        // users won't need to reset objects themselves.
        //
        // static void OnGet(T*) { ... }

        // Hook for `Put`. It's called before an object is put into the pool. It can
        // be handy if you want to release specific precious resources (handle to
        // temporary file, for example) before the object is hold by the pool.
        //
        // static void OnPut(T*) { ... }

        // For type-specific arguments, see header for the corresponding backend.

        // ... TODO

        static_assert(sizeof(T) == 0,
                      "You need to specialize `abel::object_pool::pool_traits` to "
                      "specify parameters before using `object_pool::Xxx`.");
    };

    namespace memory_internal {

        // Call corresponding backend to get an object. Hook is not called.
        template<class T>
        inline void *get_without_hook() {
            constexpr auto kType = pool_traits<T>::kType;

            if constexpr (kType == pool_type::Disabled) {
                return disabled_get(*get_type_desc<T>());
            } else if constexpr (kType == pool_type::ThreadLocal) {
                return tls_get(*get_type_desc<T>(), get_thread_local_pool<T>());
                //} else if constexpr (kType == pool_type::MemoryNodeShared) {
                //  return memory_node_shared::get<T>();
            } else if constexpr (kType == pool_type::Global) {
                return global_get(*get_type_desc<T>());
            } else {
                static_assert(sizeof(T) == 0, "Unexpected pool type.");
                DCHECK(0, "");
            }
        }

        // Call corresponding backend to return an object. Hook is called by the caller.
        template<class T>
        inline void put_without_hook(void *ptr) {
            constexpr auto kType = pool_traits<T>::kType;

            if constexpr (kType == pool_type::Disabled) {
                disabled_put(*get_type_desc<T>(), ptr);
            } else if constexpr (kType == pool_type::ThreadLocal) {
                tls_put(*get_type_desc<T>(), get_thread_local_pool<T>(), ptr);
                // } else if constexpr (kType == pool_type::MemoryNodeShared) {
                //    return memory_node_shared::Put<T>(ptr);
            } else if constexpr (kType == pool_type::Global) {
                global_put(*get_type_desc<T>(), ptr);
            } else {
                static_assert(sizeof(T) == 0, "Unexpected pool type.");
                DCHECK(0, "");
            }
        }

        // get an object from the corresponding backend.
        template<class T>
        inline void *get() {
            auto rc = get_without_hook<T>();
            OnGetHook<T>(rc);
            return rc;
        }

        // Put an object to the corresponding backend.
        template<class T>
        inline void put(void *ptr) {
            DCHECK(ptr,
                       "I'm pretty sure null pointer is not what you got when you "
                       "called `get`.");
            OnPutHook<T>(ptr);
            return put_without_hook<T>(ptr);
        }

    }  // namespace memory_internal

    // RAII wrapper for resources allocated from object pool.
    template<class T>
    class pooled_ptr final {
    public:
        constexpr pooled_ptr();

        ~pooled_ptr();

        /* implicit */ constexpr pooled_ptr(std::nullptr_t) : pooled_ptr() {}

        // Used by `get<T>()`. You don't want to call this normally.
        constexpr explicit pooled_ptr(T *ptr) : ptr_(ptr) {}

        // Movable but not copyable.
        constexpr pooled_ptr(pooled_ptr &&ptr) noexcept;

        constexpr pooled_ptr &operator=(pooled_ptr &&ptr) noexcept;

        // Test if the pointer is nullptr.
        constexpr explicit operator bool() const noexcept;

        // Accessor.
        constexpr T *get() const noexcept;

        constexpr T *operator->() const noexcept;

        constexpr T &operator*() const noexcept;

        // Equivalent to `Reset(nullptr)`.
        constexpr pooled_ptr &operator=(std::nullptr_t) noexcept;

        // `ptr` must be obtained from calling `Leak()` on another `pooled_ptr`.
        constexpr void reset(T *ptr = nullptr) noexcept;

        // Ownership is transferred to caller.
        [[nodiscard]] constexpr T *leak() noexcept;

    private:
        T *ptr_;
    };

    namespace object_pool {

        template<class T>
        pooled_ptr<T> get() {
            return pooled_ptr<T>(static_cast<T *>(memory_internal::get<T>()));
        }

        template<class T>
        void put(std::common_type_t<T> *ptr) {
            return abel::memory_internal::put<T>(ptr);
        }

    }  // namespace object_pool

    namespace memory_internal {

        void InitializeObjectPoolForCurrentThread();

    }  // namespace internal

    template<class T>
    constexpr pooled_ptr<T>::pooled_ptr() : ptr_(nullptr) {}

    template<class T>
    pooled_ptr<T>::~pooled_ptr() {
        if (ptr_) {
            object_pool::put<T>(ptr_);
        }
    }

    template<class T>
    constexpr pooled_ptr<T>::pooled_ptr(pooled_ptr &&ptr) noexcept : ptr_(ptr.ptr_) {
        ptr.ptr_ = nullptr;
    }

    template<class T>
    constexpr pooled_ptr<T> &pooled_ptr<T>::operator=(pooled_ptr &&ptr) noexcept {
        if (ABEL_LIKELY(this != &ptr)) {
            reset(ptr.Leak());
        }
        return *this;
    }

    template<class T>
    constexpr pooled_ptr<T>::operator bool() const noexcept {
        return !!ptr_;
    }

    template<class T>
    constexpr T *pooled_ptr<T>::get() const noexcept {
        return ptr_;
    }

    template<class T>
    constexpr T *pooled_ptr<T>::operator->() const noexcept {
        return ptr_;
    }

    template<class T>
    constexpr T &pooled_ptr<T>::operator*() const noexcept {
        return *ptr_;
    }

    template<class T>
    constexpr pooled_ptr<T> &pooled_ptr<T>::operator=(std::nullptr_t) noexcept {
        reset(nullptr);
        return *this;
    }

    template<class T>
    constexpr void pooled_ptr<T>::reset(T *ptr) noexcept {
        if (ptr_) {
            object_pool::put<T>(ptr_);
        }
        ptr_ = ptr;
    }

    template<class T>
    constexpr T *pooled_ptr<T>::leak() noexcept {
        return std::exchange(ptr_, nullptr);
    }

    template<class T>
    constexpr bool operator==(const pooled_ptr<T> &ptr, std::nullptr_t) {
        return ptr.get() == nullptr;
    }

    template<class T>
    constexpr bool operator==(std::nullptr_t, const pooled_ptr<T> &ptr) {
        return ptr.get() == nullptr;
    }

}  // namespace abel

#endif  // ABEL_MEMORY_OBJECT_POOL_H_
