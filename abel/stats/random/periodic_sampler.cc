//
// Created by liyinbin on 2020/2/27.
//

#include <abel/stats/random/periodic_sampler.h>
#include <atomic>

namespace abel {

    int64_t periodic_sampler_base::get_exponential_biased(int pd) noexcept {
        return _rng.get_stride(pd);
    }

    bool periodic_sampler_base::subtle_confirm_sample() noexcept {
        int current_period = period();

        // Deal with period case 0 (always off) and 1 (always on)
        if (ABEL_UNLIKELY(current_period < 2)) {
            _stride = 0;
            return current_period == 1;
        }

        // Check if this is the first call to Sample()
        if (ABEL_UNLIKELY(_stride == 1)) {
            _stride = static_cast<uint64_t>(-get_exponential_biased(current_period));
            if (static_cast<int64_t>(_stride) < -1) {
                ++_stride;
                return false;
            }
        }

        _stride = static_cast<uint64_t>(-get_exponential_biased(current_period));
        return true;
    }


}