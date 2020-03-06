//
#ifndef ABEL_RANDOM_INTERNAL_UNIFORM_HELPER_H_
#define ABEL_RANDOM_INTERNAL_UNIFORM_HELPER_H_

#include <cmath>
#include <limits>
#include <type_traits>

#include <abel/meta/type_traits.h>

namespace abel {

    template<typename IntType>
    class uniform_int_distribution;

    template<typename RealType>
    class uniform_real_distribution;

// Interval tag types which specify whether the interval is open or closed
// on either boundary.

    namespace random_internal {
        template<typename T>
        struct tag_type_compare {
        };

        template<typename T>
        constexpr bool operator==(tag_type_compare<T>, tag_type_compare<T>) {
            // Tags are mono-states. They always compare equal.
            return true;
        }

        template<typename T>
        constexpr bool operator!=(tag_type_compare<T>, tag_type_compare<T>) {
            return false;
        }

    }  // namespace random_internal

    struct interval_closed_closed_tag
            : public random_internal::tag_type_compare<interval_closed_closed_tag> {
    };
    struct interval_closed_open_tag
            : public random_internal::tag_type_compare<interval_closed_open_tag> {
    };
    struct interval_open_closed_tag
            : public random_internal::tag_type_compare<interval_open_closed_tag> {
    };
    struct interval_open_open_tag
            : public random_internal::tag_type_compare<interval_open_open_tag> {
    };

    namespace random_internal {
// The functions
//    uniform_lower_bound(tag, a, b)
// and
//    uniform_upper_bound(tag, a, b)
// are used as implementation-details for abel::uniform().
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
        template<typename IntType, typename Tag>
        typename abel::enable_if_t<
                abel::conjunction<
                        std::is_integral<IntType>,
                        abel::disjunction<std::is_same<Tag, interval_open_closed_tag>,
                                std::is_same<Tag, interval_open_open_tag>>>::value,
                IntType>
        uniform_lower_bound(Tag, IntType a, IntType) {
            return a + 1;
        }

        template<typename FloatType, typename Tag>
        typename abel::enable_if_t<
                abel::conjunction<
                        std::is_floating_point<FloatType>,
                        abel::disjunction<std::is_same<Tag, interval_open_closed_tag>,
                                std::is_same<Tag, interval_open_open_tag>>>::value,
                FloatType>
        uniform_lower_bound(Tag, FloatType a, FloatType b) {
            return std::nextafter(a, b);
        }

        template<typename NumType, typename Tag>
        typename abel::enable_if_t<
                abel::disjunction<std::is_same<Tag, interval_closed_closed_tag>,
                        std::is_same<Tag, interval_closed_open_tag>>::value,
                NumType>
        uniform_lower_bound(Tag, NumType a, NumType) {
            return a;
        }

        template<typename IntType, typename Tag>
        typename abel::enable_if_t<
                abel::conjunction<
                        std::is_integral<IntType>,
                        abel::disjunction<std::is_same<Tag, interval_closed_open_tag>,
                                std::is_same<Tag, interval_open_open_tag>>>::value,
                IntType>
        uniform_upper_bound(Tag, IntType, IntType b) {
            return b - 1;
        }

        template<typename FloatType, typename Tag>
        typename abel::enable_if_t<
                abel::conjunction<
                        std::is_floating_point<FloatType>,
                        abel::disjunction<std::is_same<Tag, interval_closed_open_tag>,
                                std::is_same<Tag, interval_open_open_tag>>>::value,
                FloatType>
        uniform_upper_bound(Tag, FloatType, FloatType b) {
            return b;
        }

        template<typename IntType, typename Tag>
        typename abel::enable_if_t<
                abel::conjunction<
                        std::is_integral<IntType>,
                        abel::disjunction<std::is_same<Tag, interval_closed_closed_tag>,
                                std::is_same<Tag, interval_open_closed_tag>>>::value,
                IntType>
        uniform_upper_bound(Tag, IntType, IntType b) {
            return b;
        }

        template<typename FloatType, typename Tag>
        typename abel::enable_if_t<
                abel::conjunction<
                        std::is_floating_point<FloatType>,
                        abel::disjunction<std::is_same<Tag, interval_closed_closed_tag>,
                                std::is_same<Tag, interval_open_closed_tag>>>::value,
                FloatType>
        uniform_upper_bound(Tag, FloatType, FloatType b) {
            return std::nextafter(b, (std::numeric_limits<FloatType>::max)());
        }

        template<typename NumType>
        using uniform_distribution =
        typename std::conditional<std::is_integral<NumType>::value,
                abel::uniform_int_distribution<NumType>,
                abel::uniform_real_distribution<NumType>>::type;

        template<typename NumType>
        struct uniform_distribution_wrapper : public uniform_distribution<NumType> {
            template<typename TagType>
            explicit uniform_distribution_wrapper(TagType, NumType lo, NumType hi)
                    : uniform_distribution<NumType>(
                    uniform_lower_bound<NumType>(TagType{}, lo, hi),
                    uniform_upper_bound<NumType>(TagType{}, lo, hi)) {}

            explicit uniform_distribution_wrapper(NumType lo, NumType hi)
                    : uniform_distribution<NumType>(
                    uniform_lower_bound<NumType>(interval_closed_open_tag(), lo, hi),
                    uniform_upper_bound<NumType>(interval_closed_open_tag(), lo, hi)) {}

            explicit uniform_distribution_wrapper()
                    : uniform_distribution<NumType>(std::numeric_limits<NumType>::lowest(),
                                                   (std::numeric_limits<NumType>::max)()) {}
        };

    }  // namespace random_internal

}  // namespace abel

#endif  // ABEL_RANDOM_INTERNAL_UNIFORM_HELPER_H_
