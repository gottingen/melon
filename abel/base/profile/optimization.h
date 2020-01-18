//
// -----------------------------------------------------------------------------
// File: optimization.h
// -----------------------------------------------------------------------------
//
// This header file defines portable macros for performance optimization.

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

// ABEL_CACHELINE_SIZE
//
// Explicitly defines the size of the L1 cache for purposes of alignment.
// Setting the cacheline size allows you to specify that certain objects be
// aligned on a cacheline boundary with `ABEL_CACHELINE_ALIGNED` declarations.
// (See below.)
//
// NOTE: this macro should be replaced with the following C++17 features, when
// those are generally available:
//
//   * `std::hardware_constructive_interference_size`
//   * `std::hardware_destructive_interference_size`
//
// See http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2016/p0154r1.html
// for more information.
#if defined(__GNUC__)
// Cache line alignment
#if defined(__i386__) || defined(__x86_64__)
#define ABEL_CACHELINE_SIZE 64
#elif defined(__powerpc64__)
#define ABEL_CACHELINE_SIZE 128
#elif defined(__aarch64__)
// We would need to read special register ctr_el0 to find out L1 dcache size.
// This value is a good estimate based on a real aarch64 machine.
#define ABEL_CACHELINE_SIZE 64
#elif defined(__arm__)
// Cache line sizes for ARM: These values are not strictly correct since
// cache line sizes depend on implementations, not architectures.  There
// are even implementations with cache line sizes configurable at boot
// time.
#if defined(__ARM_ARCH_5T__)
#define ABEL_CACHELINE_SIZE 32
#elif defined(__ARM_ARCH_7A__)
#define ABEL_CACHELINE_SIZE 64
#endif
#endif

#ifndef ABEL_CACHELINE_SIZE
// A reasonable default guess.  Note that overestimates tend to waste more
// space, while underestimates tend to waste more time.
#define ABEL_CACHELINE_SIZE 64
#endif

// ABEL_CACHELINE_ALIGNED
//
// Indicates that the declared object be cache aligned using
// `ABEL_CACHELINE_SIZE` (see above). Cacheline aligning objects allows you to
// load a set of related objects in the L1 cache for performance improvements.
// Cacheline aligning objects properly allows constructive memory sharing and
// prevents destructive (or "false") memory sharing.
//
// NOTE: this macro should be replaced with usage of `alignas()` using
// `std::hardware_constructive_interference_size` and/or
// `std::hardware_destructive_interference_size` when available within C++17.
//
// See http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2016/p0154r1.html
// for more information.
//
// On some compilers, `ABEL_CACHELINE_ALIGNED` expands to an `__attribute__`
// or `__declspec` attribute. For compilers where this is not known to work,
// the macro expands to nothing.
//
// No further guarantees are made here. The result of applying the macro
// to variables and types is always implementation-defined.
//
// WARNING: It is easy to use this attribute incorrectly, even to the point
// of causing bugs that are difficult to diagnose, crash, etc. It does not
// of itself guarantee that objects are aligned to a cache line.
//
// NOTE: Some compilers are picky about the locations of annotations such as
// this attribute, so prefer to put it at the beginning of your declaration.
// For example,
//
//   ABEL_CACHELINE_ALIGNED static Foo* foo = ...
//
//   class ABEL_CACHELINE_ALIGNED Bar { ...
//
// Recommendations:
//
// 1) Consult compiler documentation; this comment is not kept in sync as
//    toolchains evolve.
// 2) Verify your use has the intended effect. This often requires inspecting
//    the generated machine code.
// 3) Prefer applying this attribute to individual variables. Avoid
//    applying it to types. This tends to localize the effect.
#define ABEL_CACHELINE_ALIGNED __attribute__((aligned(ABEL_CACHELINE_SIZE)))
#elif defined(_MSC_VER)
#define ABEL_CACHELINE_SIZE 64
#define ABEL_CACHELINE_ALIGNED __declspec(align(ABEL_CACHELINE_SIZE))
#else
#define ABEL_CACHELINE_SIZE 64
#define ABEL_CACHELINE_ALIGNED
#endif

// ABEL_PREDICT_TRUE, ABEL_PREDICT_FALSE
//
// Enables the compiler to prioritize compilation using static analysis for
// likely paths within a boolean branch.
//
// Example:
//
//   if (ABEL_PREDICT_TRUE(expression)) {
//     return result;                        // Faster if more likely
//   } else {
//     return 0;
//   }
//
// Compilers can use the information that a certain branch is not likely to be
// taken (for instance, a CHECK failure) to optimize for the common case in
// the absence of better information (ie. compiling gcc with `-fprofile-arcs`).
//
// Recommendation: Modern CPUs dynamically predict branch execution paths,
// typically with accuracy greater than 97%. As a result, annotating every
// branch in a codebase is likely counterproductive; however, annotating
// specific branches that are both hot and consistently mispredicted is likely
// to yield performance improvements.
#if ABEL_COMPILER_HAS_BUILTIN(__builtin_expect) || \
    (defined(__GNUC__) && !defined(__clang__))
#define ABEL_PREDICT_FALSE(x) (__builtin_expect(x, 0))
#define ABEL_PREDICT_TRUE(x) (__builtin_expect(false || (x), true))
#else
#define ABEL_PREDICT_FALSE(x) (x)
#define ABEL_PREDICT_TRUE(x) (x)
#endif

#endif  // ABEL_BASE_OPTIMIZATION_H_
