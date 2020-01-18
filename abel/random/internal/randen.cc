//

#include <abel/random/internal/randen.h>

#include <abel/base/internal/raw_logging.h>
#include <abel/random/internal/randen_detect.h>

// RANDen = RANDom generator or beetroots in Swiss German.
// 'Strong' (well-distributed, unpredictable, backtracking-resistant) random
// generator, faster in some benchmarks than std::mt19937_64 and pcg64_c32.
//
// High-level summary:
// 1) Reverie (see "A Robust and Sponge-Like PRNG with Improved Efficiency") is
//    a sponge-like random generator that requires a cryptographic permutation.
//    It improves upon "Provably Robust Sponge-Based PRNGs and KDFs" by
//    achieving backtracking resistance with only one Permute() per buffer.
//
// 2) "Simpira v2: A Family of Efficient Permutations Using the AES Round
//    Function" constructs up to 1024-bit permutations using an improved
//    Generalized Feistel network with 2-round AES-128 functions. This Feistel
//    block shuffle achieves diffusion faster and is less vulnerable to
//    sliced-biclique attacks than the Type-2 cyclic shuffle.
//
// 3) "Improving the Generalized Feistel" and "New criterion for diffusion
//    property" extends the same kind of improved Feistel block shuffle to 16
//    branches, which enables a 2048-bit permutation.
//
// We combine these three ideas and also change Simpira's subround keys from
// structured/low-entropy counters to digits of Pi.

namespace abel {
ABEL_NAMESPACE_BEGIN
namespace random_internal {
namespace {

struct RandenState {
  const void* keys;
  bool has_crypto;
};

RandenState GetRandenState() {
  static const RandenState state = []() {
    RandenState tmp;
#if ABEL_RANDOM_INTERNAL_AES_DISPATCH
    // HW AES Dispatch.
    if (HasRandenHwAesImplementation() && CPUSupportsRandenHwAes()) {
      tmp.has_crypto = true;
      tmp.keys = RandenHwAes::GetKeys();
    } else {
      tmp.has_crypto = false;
      tmp.keys = RandenSlow::GetKeys();
    }
#elif ABEL_HAVE_ACCELERATED_AES
    // HW AES is enabled.
    tmp.has_crypto = true;
    tmp.keys = RandenHwAes::GetKeys();
#else
    // HW AES is disabled.
    tmp.has_crypto = false;
    tmp.keys = RandenSlow::GetKeys();
#endif
    return tmp;
  }();
  return state;
}

}  // namespace

Randen::Randen() {
  auto tmp = GetRandenState();
  keys_ = tmp.keys;
#if ABEL_RANDOM_INTERNAL_AES_DISPATCH
  has_crypto_ = tmp.has_crypto;
#endif
}

}  // namespace random_internal
ABEL_NAMESPACE_END
}  // namespace abel
