//
// This file is a Linux-specific part of spinlock_wait.cc

#include <linux/futex.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <atomic>
#include <cerrno>
#include <climits>
#include <cstdint>
#include <ctime>

#include <abel/base/profile.h>

// The spin_lock lockword is `std::atomic<uint32_t>`. Here we assert that
// `std::atomic<uint32_t>` is bitwise equivalent of the `int` expected
// by SYS_futex. We also assume that reads/writes done to the lockword
// by SYS_futex have rational semantics with regard to the
// std::atomic<> API. C++ provides no guarantees of these assumptions,
// but they are believed to hold in practice.
static_assert(sizeof(std::atomic<uint32_t>) == sizeof(int),
              "spin_lock lockword has the wrong size for a futex");

// Some Android headers are missing these definitions even though they
// support these futex operations.
#ifdef __BIONIC__
#ifndef SYS_futex
#define SYS_futex __NR_futex
#endif
#ifndef FUTEX_PRIVATE_FLAG
#define FUTEX_PRIVATE_FLAG 128
#endif
#endif

extern "C" {

ABEL_WEAK void abel_internal_spin_lock_delay(
        std::atomic<uint32_t> *w, uint32_t value, int loop,
        abel::thread_internal::SchedulingMode) {
    int save_errno = errno;
    struct timespec tm;
    tm.tv_sec = 0;
    tm.tv_nsec = abel::thread_internal::spin_lock_suggested_delay_ns(loop);
    syscall(SYS_futex, w, FUTEX_WAIT | FUTEX_PRIVATE_FLAG, value, &tm);
    errno = save_errno;
}

ABEL_WEAK void abel_internal_spin_lock_wake(std::atomic<uint32_t> *w,
                                        bool all) {
    syscall(SYS_futex, w, FUTEX_WAKE | FUTEX_PRIVATE_FLAG, all ? INT_MAX : 1, 0);
}

}  // extern "C"
