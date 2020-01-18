//
// This file is a Posix-specific part of spinlock_wait.cc

#include <sched.h>
#include <atomic>
#include <ctime>
#include <cerrno>

#include <abel/base/internal/scheduling_mode.h>
#include <abel/base/profile.h>

extern "C" {

ABEL_WEAK void AbelInternalSpinLockDelay(
    std::atomic<uint32_t>* /* lock_word */, uint32_t /* value */, int loop,
    abel::base_internal::SchedulingMode /* mode */) {
  int save_errno = errno;
  if (loop == 0) {
  } else if (loop == 1) {
    sched_yield();
  } else {
    struct timespec tm;
    tm.tv_sec = 0;
    tm.tv_nsec = abel::base_internal::SpinLockSuggestedDelayNS(loop);
    nanosleep(&tm, nullptr);
  }
  errno = save_errno;
}

ABEL_WEAK void AbelInternalSpinLockWake(
    std::atomic<uint32_t>* /* lock_word */, bool /* all */) {}

}  // extern "C"
