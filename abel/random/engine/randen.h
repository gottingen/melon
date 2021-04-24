//

#ifndef ABEL_RANDOM_INTERNAL_RANDEN_H_
#define ABEL_RANDOM_INTERNAL_RANDEN_H_

#include <cstddef>
#include "abel/base/profile.h"
#include "abel/random/engine/randen_hwaes.h"
#include "abel/random/engine/randen_slow.h"
#include "abel/random/engine/randen_traits.h"

namespace abel {

namespace random_internal {

// RANDen = RANDom generator or beetroots in Swiss German.
// 'Strong' (well-distributed, unpredictable, backtracking-resistant) random
// generator, faster in some benchmarks than std::mt19937_64 and pcg64_c32.
//
// randen implements the basic state manipulation methods.
class randen {
  public:
    static constexpr size_t kStateBytes = randen_traits::kStateBytes;
    static constexpr size_t kCapacityBytes = randen_traits::kCapacityBytes;
    static constexpr size_t kSeedBytes = randen_traits::kSeedBytes;

    ~randen() = default;

    randen();

    // Generate updates the randen sponge. The outer portion of the sponge
    // (kCapacityBytes .. kStateBytes) may be consumed as PRNG state.
    template<typename T, size_t N>
    void Generate(T (&state)[N]) const {
        static_assert(N * sizeof(T) == kStateBytes,
                      "randen::Generate() requires kStateBytes of state");
#if ABEL_AES_DISPATCH
        // HW AES Dispatch.
        if (has_crypto_) {
            randen_hw_aes::generate(keys_, state);
        } else {
            randen_slow::generate(keys_, state);
        }
#elif ABEL_HAVE_ACCELERATED_AES
        // HW AES is enabled.
        randen_hw_aes::Generate(keys_, state);
#else
        // HW AES is disabled.
        randen_slow::Generate(keys_, state);
#endif
    }

    // Absorb incorporates additional seed material into the randen sponge.  After
    // absorb returns, Generate must be called before the state may be consumed.
    template<typename S, size_t M, typename T, size_t N>
    void Absorb(const S (&seed)[M], T (&state)[N]) const {
        static_assert(M * sizeof(S) == randen_traits::kSeedBytes,
                      "randen::Absorb() requires kSeedBytes of seed");

        static_assert(N * sizeof(T) == randen_traits::kStateBytes,
                      "randen::Absorb() requires kStateBytes of state");
#if ABEL_AES_DISPATCH
        // HW AES Dispatch.
        if (has_crypto_) {
            randen_hw_aes::absorb(seed, state);
        } else {
            randen_slow::absorb(seed, state);
        }
#elif ABEL_HAVE_ACCELERATED_AES
        // HW AES is enabled.
        randen_hw_aes::Absorb(seed, state);
#else
        // HW AES is disabled.
        randen_slow::Absorb(seed, state);
#endif
    }

  private:
    const void *keys_;
#if ABEL_AES_DISPATCH
    bool has_crypto_;
#endif
};

}  // namespace random_internal

}  // namespace abel

#endif  // ABEL_RANDOM_INTERNAL_RANDEN_H_
