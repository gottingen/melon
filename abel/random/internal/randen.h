//

#ifndef ABEL_RANDOM_INTERNAL_RANDEN_H_
#define ABEL_RANDOM_INTERNAL_RANDEN_H_

#include <cstddef>
#include <abel/base/profile.h>
#include <abel/random/internal/randen_hwaes.h>
#include <abel/random/internal/randen_slow.h>
#include <abel/random/internal/randen_traits.h>

namespace abel {

    namespace random_internal {

// RANDen = RANDom generator or beetroots in Swiss German.
// 'Strong' (well-distributed, unpredictable, backtracking-resistant) random
// generator, faster in some benchmarks than std::mt19937_64 and pcg64_c32.
//
// Randen implements the basic state manipulation methods.
        class Randen {
        public:
            static constexpr size_t kStateBytes = RandenTraits::kStateBytes;
            static constexpr size_t kCapacityBytes = RandenTraits::kCapacityBytes;
            static constexpr size_t kSeedBytes = RandenTraits::kSeedBytes;

            ~Randen() = default;

            Randen();

            // Generate updates the randen sponge. The outer portion of the sponge
            // (kCapacityBytes .. kStateBytes) may be consumed as PRNG state.
            template<typename T, size_t N>
            void Generate(T (&state)[N]) const {
                static_assert(N * sizeof(T) == kStateBytes,
                              "Randen::Generate() requires kStateBytes of state");
#if ABEL_AES_DISPATCH
                // HW AES Dispatch.
                if (has_crypto_) {
                    RandenHwAes::Generate(keys_, state);
                } else {
                    RandenSlow::Generate(keys_, state);
                }
#elif ABEL_HAVE_ACCELERATED_AES
                // HW AES is enabled.
                RandenHwAes::Generate(keys_, state);
#else
                // HW AES is disabled.
                RandenSlow::Generate(keys_, state);
#endif
            }

            // Absorb incorporates additional seed material into the randen sponge.  After
            // absorb returns, Generate must be called before the state may be consumed.
            template<typename S, size_t M, typename T, size_t N>
            void Absorb(const S (&seed)[M], T (&state)[N]) const {
                static_assert(M * sizeof(S) == RandenTraits::kSeedBytes,
                              "Randen::Absorb() requires kSeedBytes of seed");

                static_assert(N * sizeof(T) == RandenTraits::kStateBytes,
                              "Randen::Absorb() requires kStateBytes of state");
#if ABEL_AES_DISPATCH
                // HW AES Dispatch.
                if (has_crypto_) {
                    RandenHwAes::Absorb(seed, state);
                } else {
                    RandenSlow::Absorb(seed, state);
                }
#elif ABEL_HAVE_ACCELERATED_AES
                // HW AES is enabled.
                RandenHwAes::Absorb(seed, state);
#else
                // HW AES is disabled.
                RandenSlow::Absorb(seed, state);
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
