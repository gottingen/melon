//
// Created by liyinbin on 2020/1/29.
//

#ifndef ABEL_THREAD_THIS_THREAD_H_
#define ABEL_THREAD_THIS_THREAD_H_

#include <cstddef>

namespace abel {

    namespace this_thread {

        extern __thread size_t sys_thread_id;
        extern __thread char sys_thread_name[16];

    } //namespace this_thread

    size_t thread_id();

    int thread_atexit(void (*fn)());

    int thread_atexit(void (*fn)(void *), void *arg);

    void thread_atexit_cancel(void (*fn)());

    void thread_atexit_cancel(void (*fn)(void *), void *arg);

    template<typename T>
    void delete_object(void *arg) {
        delete static_cast<T *>(arg);
    }

} //namespace abel
#endif //ABEL_THREAD_THIS_THREAD_H_
