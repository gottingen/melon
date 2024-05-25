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

 
// Date: Thu Dec 15 14:37:39 CST 2016

#ifndef MUTIL_MEMORY_SINGLETON_ON_PTHREAD_ONCE_H
#define MUTIL_MEMORY_SINGLETON_ON_PTHREAD_ONCE_H

#include <pthread.h>
#include <melon/utility/atomicops.h>

namespace mutil {

template <typename T> class GetLeakySingleton {
public:
    static mutil::subtle::AtomicWord g_leaky_singleton_untyped;
    static pthread_once_t g_create_leaky_singleton_once;
    static void create_leaky_singleton();
};
template <typename T>
mutil::subtle::AtomicWord GetLeakySingleton<T>::g_leaky_singleton_untyped = 0;

template <typename T>
pthread_once_t GetLeakySingleton<T>::g_create_leaky_singleton_once = PTHREAD_ONCE_INIT;

template <typename T>
void GetLeakySingleton<T>::create_leaky_singleton() {
    T* obj = new T;
    mutil::subtle::Release_Store(
        &g_leaky_singleton_untyped,
        reinterpret_cast<mutil::subtle::AtomicWord>(obj));
}

// To get a never-deleted singleton of a type T, just call get_leaky_singleton<T>().
// Most daemon threads or objects that need to be always-on can be created by
// this function.
// This function can be called safely before main() w/o initialization issues of
// global variables.
template <typename T>
inline T* get_leaky_singleton() {
    const mutil::subtle::AtomicWord value = mutil::subtle::Acquire_Load(
        &GetLeakySingleton<T>::g_leaky_singleton_untyped);
    if (value) {
        return reinterpret_cast<T*>(value);
    }
    pthread_once(&GetLeakySingleton<T>::g_create_leaky_singleton_once,
                 GetLeakySingleton<T>::create_leaky_singleton);
    return reinterpret_cast<T*>(
        GetLeakySingleton<T>::g_leaky_singleton_untyped);
}

// True(non-NULL) if the singleton is created.
// The returned object (if not NULL) can be used directly.
template <typename T>
inline T* has_leaky_singleton() {
    return reinterpret_cast<T*>(
        mutil::subtle::Acquire_Load(
            &GetLeakySingleton<T>::g_leaky_singleton_untyped));
}

} // namespace mutil

#endif // MUTIL_MEMORY_SINGLETON_ON_PTHREAD_ONCE_H
