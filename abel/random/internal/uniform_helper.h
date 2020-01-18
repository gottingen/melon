//
#ifndef ABEL_RANDOM_INTERNAL_UNIFORM_HELPER_H_
#define ABEL_RANDOM_INTERNAL_UNIFORM_HELPER_H_

#include <cmath>
#include <limits>
#include <type_traits>

#include <abel/meta/type_traits.h>

namespace abel {
ABEL_NAMESPACE_BEGIN
template <typename IntType>
class uniform_int_distribution;

template <typename RealType>
class uniform_real_distribution;

// Interval tag types which specify whether the interval is open or closed
// on either boundary.

namespace random_internal {
template <typename T>
struct TagTypeCompare {};

template <typename T>
constexpr bool operator==(TagTypeCompare<T>, TagTypeCompare<T>) {
  // Tags are mono-states. They always compare equal.
  return true;
}
template <typename T>
constexpr bool operator!=(TagTypeCompare<T>, TagTypeCompare<T>) {
  return false;
}

}  // namespace random_internal

struct IntervalClosedClosedTag
    : public random_internal::TagTypeCompare<IntervalClosedClosedTag> {};
struct IntervalClosedOpenTag
    : public random_internal::TagTypeCompare<IntervalClosedOpenTag> {};
struct IntervalOpenClosedTag
    : public random_internal::TagTypeCompare<IntervalOpenClosedTag> {};
struct IntervalOpenOpenTag
    : public random_internal::TagTypeCompare<IntervalOpenOpenTag> {};

namespace random_internal {
// The functions
//    uniform_lower_bound(tag, a, b)
// and
//    uniform_upper_bound(tag, a, b)
// are used as implementation-details for abel::Uniform().
//
// Conceptually,
//    [a, b] == [uniform_lower_bound(IntervalClosedClosed, a, b),
//               uniform_upper_bound(IntervalClosedClosed, a, b)]
//    (a, b) == [uniform_lower_bound(IntervalOpenOpen, a, b),
//               uniform_upper_bound(IntervalOpenOpen, a, b)]
//    [a, b) == [uniform_lower_bound(IntervalClosedOpen, a, b),
//               uniform_upper_bound(IntervalClosedOpen, a, b)]
//    (a, b] == [uniform_lower_bound(IntervalOpenClosed, a, b),
//               uniform_upper_bound(IntervalOpenClosed, a, b)]
//
template <typename IntType, typename Tag>
typename abel::enable_if_t<
    abel::conjunction<
        std::is_integral<IntType>,
        abel::disjunction<std::is_same<Tag, IntervalOpenClosedTag>,
                          std::is_same<Tag, IntervalOpenOpenTag>>>::value,
    IntType>
uniform_lower_bound(Tag, IntType a, IntType) {
  return a + 1;
}

template <typename FloatType, typename Tag>
typename abel::enable_if_t<
    abel::conjunction<
        std::is_floating_point<FloatType>,
        abel::disjunction<std::is_same<Tag, IntervalOpenClosedTag>,
                          std::is_same<Tag, IntervalOpenOpenTag>>>::value,
    FloatType>
uniform_lower_bound(Tag, FloatType a, FloatType b) {
  return std::nextafter(a, b);
}

template <typename NumType, typename Tag>
typename abel::enable_if_t<
    abel::disjunction<std::is_same<Tag, IntervalClosedClosedTag>,
                      std::is_same<Tag, IntervalClosedOpenTag>>::value,
    NumType>
uniform_lower_bound(Tag, NumType a, NumType) {
  return a;
}

template <typename IntType, typename Tag>
typename abel::enable_if_t<
    abel::conjunction<
        std::is_integral<IntType>,
        abel::disjunction<std::is_same<Tag, IntervalClosedOpenTag>,
                          std::is_same<Tag, IntervalOpenOpenTag>>>::value,
    IntType>
uniform_upper_bound(Tag, IntType, IntType b) {
  return b - 1;
}

template <typename FloatType, typename Tag>
typename abel::enable_if_t<
    abel::conjunction<
        std::is_floating_point<FloatType>,
        abel::disjunction<std::is_same<Tag, IntervalClosedOpenTag>,
                          std::is_same<Tag, IntervalOpenOpenTag>>>::value,
    FloatType>
uniform_upper_bound(Tag, FloatType, FloatType b) {
  return b;
}

template <typename IntType, typename Tag>
typename abel::enable_if_t<
    abel::conjunction<
        std::is_integral<IntType>,
        abel::disjunction<std::is_same<Tag, IntervalClosedClosedTag>,
                          std::is_same<Tag, IntervalOpenClosedTag>>>::value,
    IntType>
uniform_upper_bound(Tag, IntType, IntType b) {
  return b;
}

template <typename FloatType, typename Tag>
typename abel::enable_if_t<
    abel::conjunction<
        std::is_floating_point<FloatType>,
        abel::disjunction<std::is_same<Tag, IntervalClosedClosedTag>,
                          std::is_same<Tag, IntervalOpenClosedTag>>>::value,
    FloatType>
uniform_upper_bound(Tag, FloatType, FloatType b) {
  return std::nextafter(b, (std::numeric_limits<FloatType>::max)());
}

template <typename NumType>
using UniformDistribution =
    typename std::conditional<std::is_integral<NumType>::value,
                              abel::uniform_int_distribution<NumType>,
                              abel::uniform_real_distribution<NumType>>::type;

template <typename NumType>
struct UniformDistributionWrapper : public UniformDistribution<NumType> {
  template <typename TagType>
  explicit UniformDistributionWrapper(TagType, NumType lo, NumType hi)
      : UniformDistribution<NumType>(
            uniform_lower_bound<NumType>(TagType{}, lo, hi),
            uniform_upper_bound<NumType>(TagType{}, lo, hi)) {}

  explicit UniformDistributionWrapper(NumType lo, NumType hi)
      : UniformDistribution<NumType>(
            uniform_lower_bound<NumType>(IntervalClosedOpenTag(), lo, hi),
            uniform_upper_bound<NumType>(IntervalClosedOpenTag(), lo, hi)) {}

  explicit UniformDistributionWrapper()
      : UniformDistribution<NumType>(std::numeric_limits<NumType>::lowest(),
                                     (std::numeric_limits<NumType>::max)()) {}
};

}  // namespace random_internal
ABEL_NAMESPACE_END
}  // namespace abel

#endif  // ABEL_RANDOM_INTERNAL_UNIFORM_HELPER_H_
