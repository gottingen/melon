// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com


#include "abel/random/exponential_biased.h"
#include <stdint.h>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <limits>


namespace abel {

int64_t exponential_biased::get_skip_count(int64_t mean) {
    if (ABEL_UNLIKELY(!_initialized)) {
        initialize();
    }

    uint64_t rng = next_random(_rng);
    _rng = rng;

    // Take the top 26 bits as the random number
    // (This plus the 1<<58 sampling bound give a max possible step of
    // 5194297183973780480 bytes.)
    // The uint32_t cast is to prevent a (hard-to-reproduce) NAN
    // under piii debug for some binaries.
    double q = static_cast<uint32_t>(rng >> (kPrngNumBits - 26)) + 1.0;
    // Put the computed p-value through the CDF of a geometric.
    double interval = _bias + (std::log2(q) - 26) * (-std::log(2.0) * mean);
    // Very large values of interval overflow int64_t. To avoid that, we will
    // cheat and clamp any huge values to (int64_t max)/2. This is a potential
    // source of bias, but the mean would need to be such a large value that it's
    // not likely to come up. For example, with a mean of 1e18, the probability of
    // hitting this condition is about 1/1000. For a mean of 1e17, standard
    // calculators claim that this event won't happen.
    if (interval > static_cast<double>(std::numeric_limits<int64_t>::max() / 2)) {
        // Assume huge values are bias neutral, retain bias for next call.
        return std::numeric_limits<int64_t>::max() / 2;
    }
    double value = std::round(interval);
    _bias = interval - value;
    return value;
}

int64_t exponential_biased::get_stride(int64_t mean) {
    return get_skip_count(mean - 1) + 1;
}

void exponential_biased::initialize() {
    // We don't get well distributed numbers from `this` so we call NextRandom() a
    // bunch to mush the bits around. We use a global_rand to handle the case
    // where the same thread (by memory address) gets created and destroyed
    // repeatedly.
    ABEL_CONST_INIT static std::atomic<uint32_t> global_rand(0);
    uint64_t r = reinterpret_cast<uint64_t>(this) +
                 global_rand.fetch_add(1, std::memory_order_relaxed);
    for (int i = 0; i < 20; ++i) {
        r = next_random(r);
    }
    _rng = r;
    _initialized = true;
}


}  // namespace abel

