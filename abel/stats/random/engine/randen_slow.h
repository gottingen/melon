//

#ifndef ABEL_RANDOM_INTERNAL_RANDEN_SLOW_H_
#define ABEL_RANDOM_INTERNAL_RANDEN_SLOW_H_

#include <cstddef>

#include <abel/base/profile.h>

namespace abel {

    namespace random_internal {

// RANDen = RANDom generator or beetroots in Swiss German.
// randen_slow implements the basic state manipulation methods for
// architectures lacking AES hardware acceleration intrinsics.
        class randen_slow {
        public:
            // Size of the entire sponge / state for the randen PRNG.
            static constexpr size_t kStateBytes = 256;  // 2048-bit

            // Size of the 'inner' (inaccessible) part of the sponge. Larger values would
            // require more frequent calls to RandenGenerate.
            static constexpr size_t kCapacityBytes = 16;  // 128-bit

            static void generate(const void *keys, void *state_void);

            static void absorb(const void *seed_void, void *state_void);

            static const void *get_keys();
        };

    }  // namespace random_internal

}  // namespace abel

#endif  // ABEL_RANDOM_INTERNAL_RANDEN_SLOW_H_
