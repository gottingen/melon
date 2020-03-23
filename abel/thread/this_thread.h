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


} //namespace abel
#endif //ABEL_THREAD_THIS_THREAD_H_
