#ifndef ABEL_BASE_OPTIMIZATION_H_
#define ABEL_BASE_OPTIMIZATION_H_

#include <abel/base/profile/have.h>

// ABEL_BLOCK_TAIL_CALL_OPTIMIZATION
//
// Instructs the compiler to avoid optimizing tail-call recursion. Use of this
// macro is useful when you wish to preserve the existing function order within
// a stack trace for logging, debugging, or profiling purposes.
//
// Example:
//
//   int f() {
//     int result = g();
//     ABEL_BLOCK_TAIL_CALL_OPTIMIZATION();
//     return result;
//   }
#if defined(__pnacl__)
#define ABEL_BLOCK_TAIL_CALL_OPTIMIZATION() if (volatile int x = 0) { (void)x; }
#elif defined(__clang__)
// Clang will not tail call given inline volatile assembly.
#define ABEL_BLOCK_TAIL_CALL_OPTIMIZATION() __asm__ __volatile__("")
#elif defined(__GNUC__)
// GCC will not tail call given inline volatile assembly.
#define ABEL_BLOCK_TAIL_CALL_OPTIMIZATION() __asm__ __volatile__("")
#elif defined(_MSC_VER)
#include <intrin.h>
// The __nop() intrinsic blocks the optimisation.
#define ABEL_BLOCK_TAIL_CALL_OPTIMIZATION() __nop()
#else
#define ABEL_BLOCK_TAIL_CALL_OPTIMIZATION() if (volatile int x = 0) { (void)x; }
#endif

#endif  // ABEL_BASE_OPTIMIZATION_H_
