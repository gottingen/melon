//

#ifndef ABEL_RANDOM_INTERNAL_RANDEN_TRAITS_H_
#define ABEL_RANDOM_INTERNAL_RANDEN_TRAITS_H_

// HERMETIC NOTE: The randen_hwaes target must not introduce duplicate
// symbols from arbitrary system and other headers, since it may be built
// with different flags from other targets, using different levels of
// optimization, potentially introducing ODR violations.

#include <cstddef>

#include <abel/base/profile.h>

namespace abel {

    namespace random_internal {

// RANDen = RANDom generator or beetroots in Swiss German.
// 'Strong' (well-distributed, unpredictable, backtracking-resistant) random
// generator, faster in some benchmarks than std::mt19937_64 and pcg64_c32.
//
// randen_traits contains the basic algorithm traits, such as the size of the
// state, seed, sponge, etc.
        struct randen_traits {
            // Size of the entire sponge / state for the randen PRNG.
            static constexpr size_t kStateBytes = 256;  // 2048-bit

            // Size of the 'inner' (inaccessible) part of the sponge. Larger values would
            // require more frequent calls to RandenGenerate.
            static constexpr size_t kCapacityBytes = 16;  // 128-bit

            // Size of the default seed consumed by the sponge.
            static constexpr size_t kSeedBytes = kStateBytes - kCapacityBytes;

            // Largest size for which security proofs are known.
            static constexpr size_t kFeistelBlocks = 16;

            // Type-2 generalized Feistel => one round function for every two blocks.
            static constexpr size_t kFeistelFunctions = kFeistelBlocks / 2;  // = 8

            // Ensures SPRP security and two full subblock diffusions.
            // Must be > 4 * log2(kFeistelBlocks).
            static constexpr size_t kFeistelRounds = 16 + 1;
        };

    }  // namespace random_internal

}  // namespace abel

#endif  // ABEL_RANDOM_INTERNAL_RANDEN_TRAITS_H_
