//

#ifndef ABEL_RANDOM_INTERNAL_RANDEN_HWAES_H_
#define ABEL_RANDOM_INTERNAL_RANDEN_HWAES_H_

#include <abel/base/profile.h>

// HERMETIC NOTE: The randen_hwaes target must not introduce duplicate
// symbols from arbitrary system and other headers, since it may be built
// with different flags from other targets, using different levels of
// optimization, potentially introducing ODR violations.

namespace abel {

    namespace random_internal {

// RANDen = RANDom generator or beetroots in Swiss German.
// 'Strong' (well-distributed, unpredictable, backtracking-resistant) random
// generator, faster in some benchmarks than std::mt19937_64 and pcg64_c32.
//
// randen_hw_aes implements the basic state manipulation methods.
        class randen_hw_aes {
        public:
            static void generate(const void *keys, void *state_void);

            static void absorb(const void *seed_void, void *state_void);

            static const void *get_keys();
        };

// has_randen_hw_aes_implementation returns true when there is an accelerated
// implementation, and false otherwise.  If there is no implementation,
// then attempting to use it will abort the program.
        bool has_randen_hw_aes_implementation();

    }  // namespace random_internal

}  // namespace abel

#endif  // ABEL_RANDOM_INTERNAL_RANDEN_HWAES_H_
