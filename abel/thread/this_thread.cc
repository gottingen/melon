//
// Created by liyinbin on 2020/1/29.
//

#include <abel/thread/this_thread.h>
#include <cstdint>
#include <pthread.h>
#include <abel/base/profile.h>
#include <abel/system/platform_headers.h>

namespace abel {

namespace this_thread {

__thread size_t sys_thread_id {0};
__thread char   sys_thread_name [16]{
};

} //namespace this_thread

static void cache_thread_id () {
#ifdef _WIN32
    this_thread::sys_thread_id = static_cast<size_t>(::GetCurrentThreadId());
#elif __linux__
    #if defined(__ANDROID__) && defined(__ANDROID_API__) && (__ANDROID_API__ < 21)
        #define SYS_gettid __NR_gettid
    #endif
    this_thread::sys_thread_id = static_cast<size_t>(syscall(SYS_gettid));
#elif __FreeBSD__
    long tid;
    thr_self(&tid);
    this_thread::sys_thread_id = static_cast<size_t>(tid);
#elif __APPLE__
    uint64_t tid;
    pthread_threadid_np(nullptr, &tid);
    this_thread::sys_thread_id = static_cast<size_t>(tid);
#else // Default to standard C++11 (other Unix)
    this_thread::sys_thread_id = static_cast<size_t>(std::hash<std::thread::id>()(std::this_thread::get_id()));
#endif
}

size_t thread_id() {
    if (ABEL_UNLIKELY(this_thread::sys_thread_id == 0)) {
        cache_thread_id();
    }
    return this_thread::sys_thread_id;
}
}