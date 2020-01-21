//
//

// -----------------------------------------------------------------------------
// File: cycleclock.h
// -----------------------------------------------------------------------------
//
// This header file defines a `cycle_clock`, which yields the value and frequency
// of a cycle counter that increments at a rate that is approximately constant.
//
// NOTE:
//
// The cycle counter frequency is not necessarily related to the core clock
// frequency and should not be treated as such. That is, `cycle_clock` cycles are
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

#ifndef ABEL_TIME_CYCLECLOCK_H_
#define ABEL_TIME_CYCLECLOCK_H_

#include <cstdint>

#include <abel/base/profile.h>

namespace abel {

// -----------------------------------------------------------------------------
// cycle_clock
// -----------------------------------------------------------------------------
class cycle_clock {
 public:
  // cycle_clock::now()
  //
  // Returns the value of a cycle counter that counts at a rate that is
  // approximately constant.
  static int64_t now();

  // cycle_clock::frequency()
  //
  // Returns the amount by which `cycle_clock::now()` increases per second. Note
  // that this value may not necessarily match the core CPU clock frequency.
  static double frequency();

 private:
    cycle_clock() = delete;  // no instances
    cycle_clock(const cycle_clock&) = delete;
    cycle_clock& operator=(const cycle_clock&) = delete;
};

using CycleClockSourceFunc = int64_t (*)();

class CycleClockSource {
 private:
  // CycleClockSource::Register()
  //
  // Register a function that provides an alternate source for the unscaled CPU
  // cycle count value. The source function must be async signal safe, must not
  // call cycle_clock::now(), and must have a frequency that matches that of the
  // unscaled clock used by cycle_clock. A nullptr value resets cycle_clock to use
  // the default source.
  static void Register(CycleClockSourceFunc source);
};


}  // namespace abel

#endif  // ABEL_TIME_CYCLECLOCK_H_
