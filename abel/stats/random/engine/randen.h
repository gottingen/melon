//

#ifndef ABEL_STATS_RANDOM_ENGINE_RANDEN_H_
#define ABEL_STATS_RANDOM_ENGINE_RANDEN_H_

#include <cstddef>
#include <abel/base/profile.h>
#include <abel/stats/random/engine/randen_hwaes.h>
#include <abel/stats/random/engine/randen_slow.h>
#include <abel/stats/random/engine/randen_traits.h>

namespace abel {

    namespace random_internal {

// RANDen = RANDom generator or beetroots in Swiss German.
// 'Strong' (well-distributed, unpredictable, backtracking-resistant) random
// generator, faster in some benchmarks than std::mt19937_64 and pcg64_c32.
//
// Randen implements the basic state manipulation methods.
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
                              "Randen::Generate() requires kStateBytes of state");
#if ABEL_AES_DISPATCH
                // HW AES Dispatch.
                if (_has_crypto) {
                    randen_hw_aes::generate(_keys, state);
                } else {
                    randen_slow::generate(_keys, state);
                }
#elif ABEL_HAVE_ACCELERATED_AES
                // HW AES is enabled.
                randen_hw_aes::Generate(_keys, state);
#else
                // HW AES is disabled.
                RandenSlow::Generate(_keys, state);
#endif
            }

            // Absorb incorporates additional seed material into the randen sponge.  After
            // absorb returns, Generate must be called before the state may be consumed.
            template<typename S, size_t M, typename T, size_t N>
            void Absorb(const S (&seed)[M], T (&state)[N]) const {
                static_assert(M * sizeof(S) == randen_traits::kSeedBytes,
                              "Randen::Absorb() requires kSeedBytes of seed");

                static_assert(N * sizeof(T) == randen_traits::kStateBytes,
                              "Randen::Absorb() requires kStateBytes of state");
#if ABEL_AES_DISPATCH
                // HW AES Dispatch.
                if (_has_crypto) {
                    randen_hw_aes::absorb(seed, state);
                } else {
                    randen_slow::absorb(seed, state);
                }
#elif ABEL_HAVE_ACCELERATED_AES
                // HW AES is enabled.
                randen_hw_aes::Absorb(seed, state);
#else
                // HW AES is disabled.
                RandenSlow::Absorb(seed, state);
#endif
            }

        private:
            const void *_keys;
#if ABEL_AES_DISPATCH
            bool _has_crypto;
#endif
        };

    }  // namespace random_internal

}  // namespace abel

#endif  // ABEL_STATS_RANDOM_ENGINE_RANDEN_H_
