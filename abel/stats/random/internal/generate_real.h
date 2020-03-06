//

#ifndef ABEL_RANDOM_INTERNAL_GENERATE_REAL_H_
#define ABEL_RANDOM_INTERNAL_GENERATE_REAL_H_

// This file contains some implementation details which are used by one or more
// of the abel random number distributions.

#include <cstdint>
#include <cstring>
#include <limits>
#include <type_traits>
#include <abel/meta/type_traits.h>
#include <abel/math/countl_zero.h>

namespace abel {

    namespace random_internal {

// Tristate tag types controlling the output of generate_real_from_bits.
        struct generate_positive_tag {
        };
        struct generate_negative_tag {
        };
        struct generate_signed_tag {
        };

// generate_real_from_bits generates a single real value from a single 64-bit
// `bits` with template fields controlling the output.
//
// The `SignedTag` parameter controls whether positive, negative,
// or either signed/unsigned may be returned.
//   When SignedTag == generate_positive_tag, range is U(0, 1)
//   When SignedTag == generate_negative_tag, range is U(-1, 0)
//   When SignedTag == generate_signed_tag, range is U(-1, 1)
//
// When the `IncludeZero` parameter is true, the function may return 0 for some
// inputs, otherwise it never returns 0.
//
// When a value in U(0,1) is required, use:
//   Uniform64ToReal<double, PositiveValueT, true>;
//
// When a value in U(-1,1) is required, use:
//   Uniform64ToReal<double, SignedValueT, false>;
//
//   This generates more distinct values than the mathematical equivalent
//   `U(0, 1) * 2.0 - 1.0`.
//
// Scaling the result by powers of 2 (and avoiding a multiply) is also possible:
//   generate_real_from_bits<double>(..., -1);  => U(0, 0.5)
//   generate_real_from_bits<double>(..., 1);   => U(0, 2)
//
        template<typename RealType,  // Real type, either float or double.
                typename SignedTag = generate_positive_tag,  // Whether a positive,
                // negative, or signed
                // value is generated.
                bool IncludeZero = true>
        ABEL_FORCE_INLINE RealType generate_real_from_bits(uint64_t bits, int exp_bias = 0) {
            using real_type = RealType;
            using uint_type = abel::conditional_t<std::is_same<real_type, float>::value,
                    uint32_t, uint64_t>;

            static_assert(
                    (std::is_same<double, real_type>::value ||
                     std::is_same<float, real_type>::value),
                    "generate_real_from_bits must be parameterized by either float or double.");

            static_assert(sizeof(uint_type) == sizeof(real_type),
                          "Mismatched unsinged and real types.");

            static_assert((std::numeric_limits<real_type>::is_iec559 &&
                           std::numeric_limits<real_type>::radix == 2),
                          "RealType representation is not IEEE 754 binary.");

            static_assert((std::is_same<SignedTag, generate_positive_tag>::value ||
                           std::is_same<SignedTag, generate_negative_tag>::value ||
                           std::is_same<SignedTag, generate_signed_tag>::value),
                          "");

            static constexpr int kExp = std::numeric_limits<real_type>::digits - 1;
            static constexpr uint_type kMask = (static_cast<uint_type>(1) << kExp) - 1u;
            static constexpr int kUintBits = sizeof(uint_type) * 8;

            int exp = exp_bias + int{std::numeric_limits<real_type>::max_exponent - 2};

            // Determine the sign bit.
            // Depending on the SignedTag, this may use the left-most bit
            // or it may be a constant value.
            uint_type sign = std::is_same<SignedTag, generate_negative_tag>::value
                             ? (static_cast<uint_type>(1) << (kUintBits - 1))
                             : 0;
            if (std::is_same<SignedTag, generate_signed_tag>::value) {
                if (std::is_same<uint_type, uint64_t>::value) {
                    sign = bits & uint64_t{0x8000000000000000};
                }
                if (std::is_same<uint_type, uint32_t>::value) {
                    const uint64_t tmp = bits & uint64_t{0x8000000000000000};
                    sign = static_cast<uint32_t>(tmp >> 32);
                }
                // adjust the bits and the exponent to account for removing
                // the leading bit.
                bits = bits & uint64_t{0x7FFFFFFFFFFFFFFF};
                exp++;
            }
            if (IncludeZero) {
                if (bits == 0u) return 0;
            }

            // Number of leading zeros is mapped to the exponent: 2^-clz
            // bits is 0..01xxxxxx. After shifting, we're left with 1xxx...0..0
            auto clz = abel::countl_zero(bits);
            bits <<= (IncludeZero ? clz : (clz & 63));  // remove 0-bits.
            exp -= clz;                                 // set the exponent.
            bits >>= (63 - kExp);

            // Construct the 32-bit or 64-bit IEEE 754 floating-point value from
            // the individual fields: sign, exp, mantissa(bits).
            uint_type val =
                    (std::is_same<SignedTag, generate_positive_tag>::value ? 0u : sign) |
                    (static_cast<uint_type>(exp) << kExp) |
                    (static_cast<uint_type>(bits) & kMask);

            // bit_cast to the output-type
            real_type result;
            memcpy(static_cast<void *>(&result), static_cast<const void *>(&val),
                   sizeof(result));
            return result;
        }

    }  // namespace random_internal

}  // namespace abel

#endif  // ABEL_RANDOM_INTERNAL_GENERATE_REAL_H_
