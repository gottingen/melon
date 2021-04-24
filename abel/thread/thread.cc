

#include "abel/thread/thread.h"
#include "abel/base/profile.h"
#include <algorithm>  // std::sort
#include <unordered_set>
#include <cstdarg>
#include <cstdio>

#if defined(__APPLE__)


#include <mach/thread_act.h>
#include <pthread.h>
#include <unistd.h>
#include <thread>

#elif defined(__FreeBSD__)
#include <pthread.h>
#include <pthread_np.h>
#include <unistd.h>
#include <thread>
#else
#include <pthread.h>
#include <unistd.h>
#include <thread>
#endif

namespace abel {

    class thread::thread_impl {
    public:
        thread_impl(core_affinity &&affinity, thread::Func &&f)
                : affinity(std::move(affinity)), func(std::move(f)), thread([this] {
            set_affinity();
            func();
        }) {}

        core_affinity affinity;
        Func func;
        std::thread thread;

        void set_affinity() {
            auto count = affinity.count();
            if (count == 0) {
                return;
            }

#if defined(__linux__) && !defined(__ANDROID__)
            cpu_set_t cpuset;
            CPU_ZERO(&cpuset);
            for (size_t i = 0; i < count; i++) {
              CPU_SET(affinity[i].index, &cpuset);
            }
            auto thread = pthread_self();
            pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
#elif defined(__FreeBSD__)
            cpuset_t cpuset;
            CPU_ZERO(&cpuset);
            for (size_t i = 0; i < count; i++) {
              CPU_SET(affinity[i].index, &cpuset);
            }
            auto thread = pthread_self();
            pthread_setaffinity_np(thread, sizeof(cpuset_t), &cpuset);
#else
            ABEL_ASSERT_MSG(!abel::core_affinity::supported,
                            "Attempting to use thread affinity on a unsupported platform");
#endif
        }
    };

    thread::thread(core_affinity &&affinity, Func &&func)
            : _impl(new thread::thread_impl(std::move(affinity), std::move(func))) {}

    thread::~thread() {
        ABEL_ASSERT_MSG(!_impl, "thread::join() was not called before destruction");
    }

    void thread::join() {
        _impl->thread.join();
        delete _impl;
        _impl = nullptr;
    }

    void thread::set_name(const char *fmt, ...) {
        char name[1024];
        va_list vararg;
        va_start(vararg, fmt);
        vsnprintf(name, sizeof(name), fmt, vararg);
        va_end(vararg);

#if defined(__APPLE__)
        pthread_setname_np(name);
#elif defined(__FreeBSD__)
        pthread_set_name_np(pthread_self(), name);
#elif !defined(__Fuchsia__)
        pthread_setname_np(pthread_self(), name);
#endif

    }


    thread::thread(thread &&rhs) : _impl(rhs._impl) {
        rhs._impl = nullptr;
    }

    thread &thread::operator=(thread &&rhs) {
        if (_impl) {
            delete _impl;
            _impl = nullptr;
        }
        _impl = rhs._impl;
        rhs._impl = nullptr;
        return *this;
    }

}  // namespace abel
