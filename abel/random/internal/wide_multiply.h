//

#ifndef ABEL_RANDOM_INTERNAL_WIDE_MULTIPLY_H_
#define ABEL_RANDOM_INTERNAL_WIDE_MULTIPLY_H_

#include <cstdint>
#include <limits>
#include <type_traits>

#if (defined(_WIN32) || defined(_WIN64)) && defined(_M_IA64)
#include <intrin.h>  // NOLINT(build/include_order)
#pragma intrinsic(_umul128)
#define ABEL_INTERNAL_USE_UMUL128 1
#endif

#include "abel/base/profile.h"
#include "abel/numeric/int128.h"
#include "abel/meta/type_traits.h"

namespace abel {

namespace random_internal {

// Helper object to multiply two 64-bit values to a 128-bit value.
// MultiplyU64ToU128 multiplies two 64-bit values to a 128-bit value.
// If an intrinsic is available, it is used, otherwise use native 32-bit
// multiplies to construct the result.
ABEL_FORCE_INLINE uint128 MultiplyU64ToU128(uint64_t a, uint64_t b) {
#if defined(ABEL_HAVE_INTRINSIC_INT128)
    return uint128(static_cast<__uint128_t>(a) * b);
#elif defined(ABEL_INTERNAL_USE_UMUL128)
    // uint64_t * uint64_t => uint128 multiply using imul intrinsic on MSVC.
    uint64_t high = 0;
    const uint64_t low = _umul128(a, b, &high);
    return abel::make_uint128(high, low);
#else
    // uint128(a) * uint128(b) in emulated mode computes a full 128-bit x 128-bit
    // multiply.  However there are many cases where that is not necessary, and it
    // is only necessary to support a 64-bit x 64-bit = 128-bit multiply.  This is
    // for those cases.
    const uint64_t a00 = static_cast<uint32_t>(a);
    const uint64_t a32 = a >> 32;
    const uint64_t b00 = static_cast<uint32_t>(b);
    const uint64_t b32 = b >> 32;

    const uint64_t c00 = a00 * b00;
    const uint64_t c32a = a00 * b32;
    const uint64_t c32b = a32 * b00;
    const uint64_t c64 = a32 * b32;

    const uint32_t carry =
        static_cast<uint32_t>(((c00 >> 32) + static_cast<uint32_t>(c32a) +
                               static_cast<uint32_t>(c32b)) >>
                              32);

    return abel::make_uint128(c64 + (c32a >> 32) + (c32b >> 32) + carry,
                             c00 + (c32a << 32) + (c32b << 32));
#endif
}

// wide_multiply<T> multiplies two N-bit values to a 2N-bit result.
template<typename UIntType>
struct wide_multiply {
    static constexpr size_t kN = std::numeric_limits<UIntType>::digits;
    using input_type = UIntType;
    using result_type = typename unsigned_bits<kN * 2>::type;

    static result_type multiply(input_type a, input_type b) {
        return static_cast<result_type>(a) * b;
    }

    static input_type hi(result_type r) { return r >> kN; }

    static input_type lo(result_type r) { return r; }

    static_assert(std::is_unsigned<UIntType>::value,
                  "Class-template wide_multiply<> argument must be unsigned.");
};

#ifndef ABEL_HAVE_INTRINSIC_INT128
template <>
struct wide_multiply<uint64_t> {
  using input_type = uint64_t;
  using result_type = uint128;

  static result_type multiply(uint64_t a, uint64_t b) {
    return MultiplyU64ToU128(a, b);
  }

  static uint64_t hi(result_type r) { return uint128_high64(r); }
  static uint64_t lo(result_type r) { return uint128_low64(r); }
};
#endif

}  // namespace random_internal

}  // namespace abel

#endif  // ABEL_RANDOM_INTERNAL_WIDE_MULTIPLY_H_