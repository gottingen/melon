//

#include <abel/time/unscaledcycleclock.h>

#if ABEL_USE_UNSCALED_CYCLECLOCK

#if defined(_WIN32)
#include <intrin.h>
#endif

#if defined(__powerpc__) || defined(__ppc__)
#include <sys/platform/ppc.h>
#endif

#include <abel/base/internal/sysinfo.h>

namespace abel {


#if defined(__i386__)

int64_t unscaled_cycle_clock::now() {
  int64_t ret;
  __asm__ volatile("rdtsc" : "=A"(ret));
  return ret;
}

double unscaled_cycle_clock::frequency() {
  return NominalCPUFrequency();
}

#elif defined(__x86_64__)

int64_t unscaled_cycle_clock::now() {
  uint64_t low, high;
  __asm__ volatile("rdtsc" : "=a"(low), "=d"(high));
  return (high << 32) | low;
}

double unscaled_cycle_clock::frequency() {
  return NominalCPUFrequency();
}

#elif defined(__powerpc__) || defined(__ppc__)

int64_t unscaled_cycle_clock::now() {
  return __ppc_get_timebase();
}

double unscaled_cycle_clock::Frequency() {
  return __ppc_get_timebase_freq();
}

#elif defined(__aarch64__)

// System timer of ARMv8 runs at a different frequency than the CPU's.
// The frequency is fixed, typically in the range 1-50MHz.  It can be
// read at CNTFRQ special register.  We assume the OS has set up
// the virtual timer properly.
int64_t unscaled_cycle_clock::now() {
  int64_t virtual_timer_value;
  asm volatile("mrs %0, cntvct_el0" : "=r"(virtual_timer_value));
  return virtual_timer_value;
}

double unscaled_cycle_clock::frequency() {
  uint64_t aarch64_timer_frequency;
  asm volatile("mrs %0, cntfrq_el0" : "=r"(aarch64_timer_frequency));
  return aarch64_timer_frequency;
}

#elif defined(_M_IX86) || defined(_M_X64)

#pragma intrinsic(__rdtsc)

int64_t unscaled_cycle_clock::now() {
  return __rdtsc();
}

double unscaled_cycle_clock::frequency() {
  return NominalCPUFrequency();
}

#endif

}  // namespace abel

#endif  // ABEL_USE_UNSCALED_CYCLECLOCK
