//

#include <abel/base/internal/periodic_sampler.h>

#include <atomic>

#include <abel/base/internal/exponential_biased.h>

namespace abel {
ABEL_NAMESPACE_BEGIN
namespace base_internal {

int64_t PeriodicSamplerBase::GetExponentialBiased(int pd) noexcept {
  return rng_.GetStride(pd);
}

bool PeriodicSamplerBase::SubtleConfirmSample() noexcept {
  int current_period = period();

  // Deal with period case 0 (always off) and 1 (always on)
  if (ABEL_UNLIKELY(current_period < 2)) {
    stride_ = 0;
    return current_period == 1;
  }

  // Check if this is the first call to Sample()
  if (ABEL_UNLIKELY(stride_ == 1)) {
    stride_ = static_cast<uint64_t>(-GetExponentialBiased(current_period));
    if (static_cast<int64_t>(stride_) < -1) {
      ++stride_;
      return false;
    }
  }

  stride_ = static_cast<uint64_t>(-GetExponentialBiased(current_period));
  return true;
}

}  // namespace base_internal
ABEL_NAMESPACE_END
}  // namespace abel
