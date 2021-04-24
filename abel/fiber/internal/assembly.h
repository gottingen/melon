//
// Created by liyinbin on 2021/4/4.
//

#ifndef ABEL_FIBER_INTERNAL_ASSEMBLY_H_
#define ABEL_FIBER_INTERNAL_ASSEMBLY_H_

#include <cstddef>
#include <cstdint>

namespace abel {
    namespace fiber_internal {

        // Emit (a series of) pause(s) to relax CPU.
        //
        // This can be used to delay execution for some time, or backoff from contention
        // in case you're doing some "lock-free" algorithm.
        template<std::size_t N = 1>
        [[gnu::always_inline]] inline void pause() {
            if constexpr (N != 0) {
                pause<N - 1>();
#if defined(__x86_64__)
                asm volatile("pause":: : "memory");  // x86-64 only.
#elif defined(__aarch64__)
                asm volatile("yield" ::: "memory");
#elif defined(__powerpc__)
    // FIXME: **RATHER** slow.
    asm volatile("or 31,31,31   # very low priority" ::: "memory");
#else
#error Unsupported architecture.
#endif
            }
        }

        // GCC's builtin `__builtin_popcount` won't generated `popcnt` unless compiled
        // with at least `-march=corei7`, so we use assembly here.
        //
        // `popcnt` is an SSE4.2 instruction, and should have already been widely
        // supported.
        inline int count_non_zeros(std::uint64_t value) {
#if defined(__x86_64__)
            std::uint64_t rc;
            asm volatile("popcnt %1, %0;" : "=r"(rc) : "r"(value));
            return rc;
#else
            return __builtin_popcount(value);
#endif
        }
    }
}
#endif  // ABEL_FIBER_INTERNAL_ASSEMBLY_H_
