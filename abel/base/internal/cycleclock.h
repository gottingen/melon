//
//

// -----------------------------------------------------------------------------
// File: cycleclock.h
// -----------------------------------------------------------------------------
//
// This header file defines a `CycleClock`, which yields the value and frequency
// of a cycle counter that increments at a rate that is approximately constant.
//
// NOTE:
//
// The cycle counter frequency is not necessarily related to the core clock
// frequency and should not be treated as such. That is, `CycleClock` cycles are
// not necessarily "CPU cycles" and code should not rely on that behavior, even
// if experimentally observed.
//
// An arbitrary offset may have been added to the counter at power on.
//
// On some platforms, the rate and offset of the counter may differ
// slightly when read from different CPUs of a multiprocessor. Usually,
// we try to ensure that the operating system adjusts values periodically
// so that values agree approximately.   If you need stronger guarantees,
// consider using alternate interfaces.
//
// The CPU is not required to maintain the ordering of a cycle counter read
// with respect to surrounding instructions.

#ifndef ABEL_BASE_INTERNAL_CYCLECLOCK_H_
#define ABEL_BASE_INTERNAL_CYCLECLOCK_H_

#include <cstdint>

#include <abel/base/profile.h>

namespace abel {

namespace base_internal {

// -----------------------------------------------------------------------------
// CycleClock
// -----------------------------------------------------------------------------
class CycleClock {
 public:
  // CycleClock::now()
  //
  // Returns the value of a cycle counter that counts at a rate that is
  // approximately constant.
  static int64_t now();

  // CycleClock::Frequency()
  //
  // Returns the amount by which `CycleClock::now()` increases per second. Note
  // that this value may not necessarily match the core CPU clock frequency.
  static double Frequency();

 private:
  CycleClock() = delete;  // no instances
  CycleClock(const CycleClock&) = delete;
  CycleClock& operator=(const CycleClock&) = delete;
};

using CycleClockSourceFunc = int64_t (*)();

class CycleClockSource {
 private:
  // CycleClockSource::Register()
  //
  // Register a function that provides an alternate source for the unscaled CPU
  // cycle count value. The source function must be async signal safe, must not
  // call CycleClock::now(), and must have a frequency that matches that of the
  // unscaled clock used by CycleClock. A nullptr value resets CycleClock to use
  // the default source.
  static void Register(CycleClockSourceFunc source);
};

}  // namespace base_internal

}  // namespace abel

#endif  // ABEL_BASE_INTERNAL_CYCLECLOCK_H_
