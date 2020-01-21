#ifndef ABEL_SYSTEM_SYSINFO_H_
#define ABEL_SYSTEM_SYSINFO_H_

#ifndef _WIN32
    #include <sys/types.h>
#endif

#include <cstdint>
#include <abel/base/profile.h>

namespace abel {

// Nominal core processor cycles per second of each processor.   This is _not_
// necessarily the frequency of the cycle_clock counter (see cycleclock.h)
// Thread-safe.
double nominal_cpu_frequency ();

// Number of logical processors (hyperthreads) in system. Thread-safe.
int num_cpus ();

// Return the thread id of the current thread, as told by the system.
// No two currently-live threads implemented by the OS shall have the same ID.
// Thread ids of exited threads may be reused.   Multiple user-level threads
// may have the same thread ID if multiplexed on the same OS thread.
//
// On Linux, you may send a signal to the resulting ID with kill().  However,
// it is recommended for portability that you use pthread_kill() instead.
#ifdef _WIN32
// On Windows, process id and thread id are of the same type according to the
// return types of GetProcessId() and GetThreadId() are both DWORD, an unsigned
// 32-bit type.
using pid_t = uint32_t;
#endif
pid_t get_tid ();

}  // namespace abel


#endif  // ABEL_SYSTEM_SYSINFO_H_
