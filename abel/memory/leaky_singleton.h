// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com


#ifndef ABEL_MEMORY_LEAKY_SINGLETON_H_
#define ABEL_MEMORY_LEAKY_SINGLETON_H_


#include <pthread.h>
#include <atomic>

namespace abel {

    template<typename T>
    class leaky_singleton {
    public:
        static std::atomic<uintptr_t> g_leaky_singleton_untyped;
        static std::once_flag g_create_leaky_singleton_once;

        static void create_leaky_singleton();
    };

    template<typename T>
    std::atomic<uintptr_t> leaky_singleton<T>::g_leaky_singleton_untyped{0};
    template<typename T>
    std::once_flag leaky_singleton<T>::g_create_leaky_singleton_once;

    template<typename T>
    void leaky_singleton<T>::create_leaky_singleton() {
        T *obj = new T;
        leaky_singleton<T>::g_leaky_singleton_untyped.store(reinterpret_cast<uintptr_t>(obj));
    }

// To get a never-deleted singleton of a type T, just call get_leaky_singleton<T>().
// Most daemon threads or objects that need to be always-on can be created by
// this function.
// This function can be called safely before main() w/o initialization issues of
// global variables.
    template<typename T>
    inline T *get_leaky_singleton() {
        const uintptr_t value = leaky_singleton<T>::g_leaky_singleton_untyped.load();
        if (value) {
            return reinterpret_cast<T *>(value);
        }
        std::call_once(leaky_singleton<T>::g_create_leaky_singleton_once,
                       leaky_singleton<T>::create_leaky_singleton);
        return reinterpret_cast<T *>(leaky_singleton<T>::g_leaky_singleton_untyped.load());
    }

    // True(non-NULL) if the singleton is created.
    // The returned object (if not NULL) can be used directly.
    template<typename T>
    inline T *has_leaky_singleton() {
        return reinterpret_cast<T *>(
                leaky_singleton<T>::g_leaky_singleton_untyped.load());
    }

}  // namespace abel

#endif  // ABEL_MEMORY_LEAKY_SINGLETON_H_
