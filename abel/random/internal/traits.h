//

#ifndef ABEL_RANDOM_INTERNAL_TRAITS_H_
#define ABEL_RANDOM_INTERNAL_TRAITS_H_

#include <cstdint>
#include <limits>
#include <type_traits>

#include <abel/base/profile.h>

namespace abel {
ABEL_NAMESPACE_BEGIN
namespace random_internal {

// random_internal::is_widening_convertible<A, B>
//
// Returns whether a type A is widening-convertible to a type B.
//
// A is widening-convertible to B means:
//   A a = <any number>;
//   B b = a;
//   A c = b;
//   EXPECT_EQ(a, c);
template <typename A, typename B>
class is_widening_convertible {
  // As long as there are enough bits in the exact part of a number:
  // - unsigned can fit in float, signed, unsigned
  // - signed can fit in float, signed
  // - float can fit in float
  // So we define rank to be:
  // - rank(float) -> 2
  // - rank(signed) -> 1
  // - rank(unsigned) -> 0
  template <class T>
  static constexpr int rank() {
    return !std::numeric_limits<T>::is_integer +
           std::numeric_limits<T>::is_signed;
  }

 public:
  // If an arithmetic-type B can represent at least as many digits as a type A,
  // and B belongs to a rank no lower than A, then A can be safely represented
  // by B through a widening-conversion.
  static constexpr bool value =
      std::numeric_limits<A>::digits <= std::numeric_limits<B>::digits &&
      rank<A>() <= rank<B>();
};

// unsigned_bits<N>::type returns the unsigned int type with the indicated
// number of bits.
template <size_t N>
struct unsigned_bits;

template <>
struct unsigned_bits<8> {
  using type = uint8_t;
};
template <>
struct unsigned_bits<16> {
  using type = uint16_t;
};
template <>
struct unsigned_bits<32> {
  using type = uint32_t;
};
template <>
struct unsigned_bits<64> {
  using type = uint64_t;
};

#ifdef ABEL_HAVE_INTRINSIC_INT128
template <>
struct unsigned_bits<128> {
  using type = __uint128_t;
};
#endif

template <typename IntType>
struct make_unsigned_bits {
  using type = typename unsigned_bits<std::numeric_limits<
      typename std::make_unsigned<IntType>::type>::digits>::type;
};

}  // namespace random_internal
ABEL_NAMESPACE_END
}  // namespace abel

#endif  // ABEL_RANDOM_INTERNAL_TRAITS_H_
