
#ifndef ABEL_THREAD_THREAD_H_
#define ABEL_THREAD_THREAD_H_

#include <functional>
#include <vector>

#include "abel/thread/affinity.h"

namespace abel {

    // thread provides an OS abstraction for threads of execution.
    class thread {
    public:
        using Func = std::function<void()>;


        thread() = default;

        thread(thread &&);

        thread &operator=(thread &&);

        // Start a new thread using the given affinity that calls func.
        thread(core_affinity &&affinity, Func &&func);

        ~thread();

        // join() blocks until the thread completes.
        void join();

        // set_name() sets the name of the currently executing thread for displaying
        // in a debugger.
        static void set_name(const char *fmt, ...);


    private:
        thread(const thread &) = delete;

        thread &operator=(const thread &) = delete;

        class thread_impl;

        thread_impl *_impl = nullptr;
    };


}  // namespace abel

#endif  // ABEL_THREAD_THREAD_H_
