//
// Created by liyinbin on 2020/2/27.
//

#ifndef ABEL_EXPONENTIAL_BIASED_H
#define ABEL_EXPONENTIAL_BIASED_H

#include <stdint.h>
#include <abel/base/profile.h>

namespace abel {

    class exponential_biased {
    public:
        static constexpr int kPrngNumBits = 48;

        int64_t get_skip_count(int64_t mean);

        int64_t get_stride(int64_t mean);

        static uint64_t next_random(uint64_t rnd);
    private:
        void initialize();

        uint64_t _rng{0};
        double _bias{0};
        bool _initialized{false};

    };

    ABEL_FORCE_INLINE uint64_t exponential_biased::next_random(uint64_t rnd) {
        const uint64_t prng_mult = uint64_t{0x5DEECE66D};
        const uint64_t prng_add = 0xB;
        const uint64_t prng_mod_power = 48;
        const uint64_t prng_mod_mask =
                ~((~static_cast<uint64_t>(0)) << prng_mod_power);
        return (prng_mult * rnd + prng_add) & prng_mod_mask;
    }

} //namespace abel

#endif //ABEL_EXPONENTIAL_BIASED_H
