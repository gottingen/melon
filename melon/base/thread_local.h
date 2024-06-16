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


// Date: Mon. Nov 7 14:47:36 CST 2011

#ifndef MUTIL_THREAD_LOCAL_H
#define MUTIL_THREAD_LOCAL_H

#include <new>                      // std::nothrow
#include <cstddef>                  // NULL
#include <melon/base/macros.h>

#ifdef _MSC_VER
#define MELON_THREAD_LOCAL __declspec(thread)
#else
#define MELON_THREAD_LOCAL __thread
#endif  // _MSC_VER

#define MELON_VOLATILE_THREAD_LOCAL(type, var_name, default_value)             \
  MELON_THREAD_LOCAL type var_name = default_value;                            \
  static __attribute__((noinline, unused)) type get_##var_name(void) {         \
    asm volatile("");                                                          \
    return var_name;                                                           \
  }                                                                            \
  static __attribute__((noinline, unused)) type *get_ptr_##var_name(void) {    \
    type *ptr = &var_name;                                                     \
    asm volatile("" : "+rm"(ptr));                                             \
    return ptr;                                                                \
  }                                                                            \
  static __attribute__((noinline, unused)) void set_##var_name(type v) {       \
    asm volatile("");                                                          \
    var_name = v;                                                              \
  }

#if (defined (__aarch64__) && defined (__GNUC__)) || defined(__clang__)
// GNU compiler under aarch and Clang compiler is incorrectly caching the 
// address of thread_local variables across a suspend-point. The following
// macros used to disable the volatile thread local access optimization.
#define MELON_GET_VOLATILE_THREAD_LOCAL(var_name) get_##var_name()
#define MELON_GET_PTR_VOLATILE_THREAD_LOCAL(var_name) get_ptr_##var_name()
#define MELON_SET_VOLATILE_THREAD_LOCAL(var_name, value) set_##var_name(value)
#else
#define MELON_GET_VOLATILE_THREAD_LOCAL(var_name) var_name
#define MELON_GET_PTR_VOLATILE_THREAD_LOCAL(var_name) &##var_name
#define MELON_SET_VOLATILE_THREAD_LOCAL(var_name, value) var_name = value
#endif

namespace mutil {

// Get a thread-local object typed T. The object will be default-constructed
// at the first call to this function, and will be deleted when thread
// exits.
template <typename T> inline T* get_thread_local();

// |fn| or |fn(arg)| will be called at caller's exit. If caller is not a 
// thread, fn will be called at program termination. Calling sequence is LIFO:
// last registered function will be called first. Duplication of functions 
// are not checked. This function is often used for releasing thread-local
// resources declared with __thread which is much faster than
// pthread_getspecific or boost::thread_specific_ptr.
// Returns 0 on success, -1 otherwise and errno is set.
int thread_atexit(void (*fn)());
int thread_atexit(void (*fn)(void*), void* arg);

// Remove registered function, matched functions will not be called.
void thread_atexit_cancel(void (*fn)());
void thread_atexit_cancel(void (*fn)(void*), void* arg);

// Delete the typed-T object whose address is `arg'. This is a common function
// to thread_atexit.
template <typename T> void delete_object(void* arg) {
    delete static_cast<T*>(arg);
}

}  // namespace mutil

#include "thread_local_inl.h"

#endif  // MUTIL_THREAD_LOCAL_H
