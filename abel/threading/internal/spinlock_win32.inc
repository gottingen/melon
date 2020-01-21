//
// This file is a Win32-specific part of spinlock_wait.cc

#include <windows.h>
#include <atomic>
#include <abel/base/internal/scheduling_mode.h>

extern "C" {

void AbelInternalSpinLockDelay(std::atomic<uint32_t>* /* lock_word */,
                               uint32_t /* value */, int loop,
                               abel::base_internal::SchedulingMode /* mode */) {
  if (loop == 0) {
  } else if (loop == 1) {
    Sleep(0);
  } else {
    Sleep(abel::threading_internal::SpinLockSuggestedDelayNS(loop) / 1000000);
  }
}

void AbelInternalSpinLockWake(std::atomic<uint32_t>* /* lock_word */,
                              bool /* all */) {}

}  // extern "C"
