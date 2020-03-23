//
// -----------------------------------------------------------------------------
// conformance_profiles.h
// -----------------------------------------------------------------------------
//
// This file contains templates for representing "Regularity Profiles" and
// concisely-named versions of commonly used Regularity Profiles.
//
// A Regularity Profile is a compile-time description of the types of operations
// that a given type supports, along with properties of those operations when
// they do exist. For instance, a Regularity Profile may describe a type that
// has a move-constructor that is noexcept and a copy constructor that is not
// noexcept. This description can then be examined and passed around to other
// templates for the purposes of asserting expectations on user-defined types
// via a series trait checks, or for determining what kinds of run-time tests
// are able to be performed.
//
// Regularity Profiles are also used when creating "archetypes," which are
// minimum-conforming types that meet all of the requirements of a given
// Regularity Profile. For more information regarding archetypes, see
// "conformance_archetypes.h".

#ifndef ABEL_ASL_INTERNAL_CONFORMANCE_PROFILE_H_
#define ABEL_ASL_INTERNAL_CONFORMANCE_PROFILE_H_

#include <utility>
#include <abel/asl/type_traits.h>

// TODO(calabrese) Add support for extending profiles.

namespace abel {

    namespace types_internal {

        template<class T, class /*Enabler*/ = void>
        struct PropertiesOfImpl {
        };

        template<class T>
        struct PropertiesOfImpl<T, abel::void_t<typename T::properties>> {
            using type = typename T::properties;
        };

        template<class T>
        struct PropertiesOfImpl<T, abel::void_t<typename T::profile_alias_of>> {
            using type = typename PropertiesOfImpl<typename T::profile_alias_of>::type;
        };

        template<class T>
        struct PropertiesOf : PropertiesOfImpl<T> {
        };

        template<class T>
        using PropertiesOfT = typename PropertiesOf<T>::type;

// NOTE: These enums use this naming convention to be consistent with the
// standard trait names, which is useful since it allows us to match up each
// enum name with a corresponding trait name in macro definitions.

        enum class function_kind {
            maybe, yes, nothrow, trivial
        };

#define ABEL_INTERNAL_SPECIAL_MEMBER_FUNCTION_ENUM(name) \
  enum class name { maybe, yes, nothrow, trivial }

        ABEL_INTERNAL_SPECIAL_MEMBER_FUNCTION_ENUM(default_constructible);
        ABEL_INTERNAL_SPECIAL_MEMBER_FUNCTION_ENUM(move_constructible);
        ABEL_INTERNAL_SPECIAL_MEMBER_FUNCTION_ENUM(copy_constructible);
        ABEL_INTERNAL_SPECIAL_MEMBER_FUNCTION_ENUM(move_assignable);
        ABEL_INTERNAL_SPECIAL_MEMBER_FUNCTION_ENUM(copy_assignable);
        ABEL_INTERNAL_SPECIAL_MEMBER_FUNCTION_ENUM(destructible);

#undef ABEL_INTERNAL_SPECIAL_MEMBER_FUNCTION_ENUM

#define ABEL_INTERNAL_INTRINSIC_FUNCTION_ENUM(name) \
  enum class name { maybe, yes, nothrow }

        ABEL_INTERNAL_INTRINSIC_FUNCTION_ENUM(equality_comparable);
        ABEL_INTERNAL_INTRINSIC_FUNCTION_ENUM(inequality_comparable);
        ABEL_INTERNAL_INTRINSIC_FUNCTION_ENUM(less_than_comparable);
        ABEL_INTERNAL_INTRINSIC_FUNCTION_ENUM(less_equal_comparable);
        ABEL_INTERNAL_INTRINSIC_FUNCTION_ENUM(greater_equal_comparable);
        ABEL_INTERNAL_INTRINSIC_FUNCTION_ENUM(greater_than_comparable);

        ABEL_INTERNAL_INTRINSIC_FUNCTION_ENUM(swappable);

#undef ABEL_INTERNAL_INTRINSIC_FUNCTION_ENUM

        enum class hashable {
            maybe, yes
        };

        constexpr const char *PropertyName(hashable v) {
            return "support for std::hash";
        }

        template<
                default_constructible DefaultConstructibleValue =
                default_constructible::maybe,
                move_constructible MoveConstructibleValue = move_constructible::maybe,
                copy_constructible CopyConstructibleValue = copy_constructible::maybe,
                move_assignable MoveAssignableValue = move_assignable::maybe,
                copy_assignable CopyAssignableValue = copy_assignable::maybe,
                destructible DestructibleValue = destructible::maybe,
                equality_comparable EqualityComparableValue = equality_comparable::maybe,
                inequality_comparable InequalityComparableValue =
                inequality_comparable::maybe,
                less_than_comparable LessThanComparableValue = less_than_comparable::maybe,
                less_equal_comparable LessEqualComparableValue =
                less_equal_comparable::maybe,
                greater_equal_comparable GreaterEqualComparableValue =
                greater_equal_comparable::maybe,
                greater_than_comparable GreaterThanComparableValue =
                greater_than_comparable::maybe,
                swappable SwappableValue = swappable::maybe,
                hashable HashableValue = hashable::maybe>
        struct ConformanceProfile {
            using properties = ConformanceProfile;

            static constexpr default_constructible
                    default_constructible_support =  // NOLINT
                    DefaultConstructibleValue;

            static constexpr move_constructible move_constructible_support =  // NOLINT
                    MoveConstructibleValue;

            static constexpr copy_constructible copy_constructible_support =  // NOLINT
                    CopyConstructibleValue;

            static constexpr move_assignable move_assignable_support =  // NOLINT
                    MoveAssignableValue;

            static constexpr copy_assignable copy_assignable_support =  // NOLINT
                    CopyAssignableValue;

            static constexpr destructible destructible_support =  // NOLINT
                    DestructibleValue;

            static constexpr equality_comparable equality_comparable_support =  // NOLINT
                    EqualityComparableValue;

            static constexpr inequality_comparable
                    inequality_comparable_support =  // NOLINT
                    InequalityComparableValue;

            static constexpr less_than_comparable
                    less_than_comparable_support =  // NOLINT
                    LessThanComparableValue;

            static constexpr less_equal_comparable
                    less_equal_comparable_support =  // NOLINT
                    LessEqualComparableValue;

            static constexpr greater_equal_comparable
                    greater_equal_comparable_support =  // NOLINT
                    GreaterEqualComparableValue;

            static constexpr greater_than_comparable
                    greater_than_comparable_support =  // NOLINT
                    GreaterThanComparableValue;

            static constexpr swappable swappable_support = SwappableValue;  // NOLINT

            static constexpr hashable hashable_support = HashableValue;  // NOLINT

            static constexpr bool is_default_constructible =  // NOLINT
                    DefaultConstructibleValue != default_constructible::maybe;

            static constexpr bool is_move_constructible =  // NOLINT
                    MoveConstructibleValue != move_constructible::maybe;

            static constexpr bool is_copy_constructible =  // NOLINT
                    CopyConstructibleValue != copy_constructible::maybe;

            static constexpr bool is_move_assignable =  // NOLINT
                    MoveAssignableValue != move_assignable::maybe;

            static constexpr bool is_copy_assignable =  // NOLINT
                    CopyAssignableValue != copy_assignable::maybe;

            static constexpr bool is_destructible =  // NOLINT
                    DestructibleValue != destructible::maybe;

            static constexpr bool is_equality_comparable =  // NOLINT
                    EqualityComparableValue != equality_comparable::maybe;

            static constexpr bool is_inequality_comparable =  // NOLINT
                    InequalityComparableValue != inequality_comparable::maybe;

            static constexpr bool is_less_than_comparable =  // NOLINT
                    LessThanComparableValue != less_than_comparable::maybe;

            static constexpr bool is_less_equal_comparable =  // NOLINT
                    LessEqualComparableValue != less_equal_comparable::maybe;

            static constexpr bool is_greater_equal_comparable =  // NOLINT
                    GreaterEqualComparableValue != greater_equal_comparable::maybe;

            static constexpr bool is_greater_than_comparable =  // NOLINT
                    GreaterThanComparableValue != greater_than_comparable::maybe;

            static constexpr bool is_swappable =  // NOLINT
                    SwappableValue != swappable::maybe;

            static constexpr bool is_hashable =  // NOLINT
                    HashableValue != hashable::maybe;
        };

#define ABEL_INTERNAL_CONFORMANCE_TESTING_DATA_MEMBER_DEF_IMPL(type, name)     \
  template <default_constructible DefaultConstructibleValue,                   \
            move_constructible MoveConstructibleValue,                         \
            copy_constructible CopyConstructibleValue,                         \
            move_assignable MoveAssignableValue,                               \
            copy_assignable CopyAssignableValue,                               \
            destructible DestructibleValue,                                    \
            equality_comparable EqualityComparableValue,                       \
            inequality_comparable InequalityComparableValue,                   \
            less_than_comparable LessThanComparableValue,                      \
            less_equal_comparable LessEqualComparableValue,                    \
            greater_equal_comparable GreaterEqualComparableValue,              \
            greater_than_comparable GreaterThanComparableValue,                \
            swappable SwappableValue, hashable HashableValue>                  \
  constexpr type ConformanceProfile<                                           \
      DefaultConstructibleValue, MoveConstructibleValue,                       \
      CopyConstructibleValue, MoveAssignableValue, CopyAssignableValue,        \
      DestructibleValue, EqualityComparableValue, InequalityComparableValue,   \
      LessThanComparableValue, LessEqualComparableValue,                       \
      GreaterEqualComparableValue, GreaterThanComparableValue, SwappableValue, \
      HashableValue>::name

#define ABEL_INTERNAL_CONFORMANCE_TESTING_DATA_MEMBER_DEF(type)           \
  ABEL_INTERNAL_CONFORMANCE_TESTING_DATA_MEMBER_DEF_IMPL(type,            \
                                                         type##_support); \
  ABEL_INTERNAL_CONFORMANCE_TESTING_DATA_MEMBER_DEF_IMPL(bool, is_##type)

        ABEL_INTERNAL_CONFORMANCE_TESTING_DATA_MEMBER_DEF(default_constructible);
        ABEL_INTERNAL_CONFORMANCE_TESTING_DATA_MEMBER_DEF(move_constructible);
        ABEL_INTERNAL_CONFORMANCE_TESTING_DATA_MEMBER_DEF(copy_constructible);
        ABEL_INTERNAL_CONFORMANCE_TESTING_DATA_MEMBER_DEF(move_assignable);
        ABEL_INTERNAL_CONFORMANCE_TESTING_DATA_MEMBER_DEF(copy_assignable);
        ABEL_INTERNAL_CONFORMANCE_TESTING_DATA_MEMBER_DEF(destructible);
        ABEL_INTERNAL_CONFORMANCE_TESTING_DATA_MEMBER_DEF(equality_comparable);
        ABEL_INTERNAL_CONFORMANCE_TESTING_DATA_MEMBER_DEF(inequality_comparable);
        ABEL_INTERNAL_CONFORMANCE_TESTING_DATA_MEMBER_DEF(less_than_comparable);
        ABEL_INTERNAL_CONFORMANCE_TESTING_DATA_MEMBER_DEF(less_equal_comparable);
        ABEL_INTERNAL_CONFORMANCE_TESTING_DATA_MEMBER_DEF(greater_equal_comparable);
        ABEL_INTERNAL_CONFORMANCE_TESTING_DATA_MEMBER_DEF(greater_than_comparable);
        ABEL_INTERNAL_CONFORMANCE_TESTING_DATA_MEMBER_DEF(swappable);
        ABEL_INTERNAL_CONFORMANCE_TESTING_DATA_MEMBER_DEF(hashable);

#undef ABEL_INTERNAL_CONFORMANCE_TESTING_DATA_MEMBER_DEF
#undef ABEL_INTERNAL_CONFORMANCE_TESTING_DATA_MEMBER_DEF_IMPL

// Converts an enum to its underlying integral value.
        template<class Enum>
        constexpr abel::underlying_type_t<Enum> UnderlyingValue(Enum value) {
            return static_cast<abel::underlying_type_t<Enum>>(value);
        }

// Retrieve the enum with the greatest underlying value.
// Note: std::max is not constexpr in C++11, which is why this is necessary.
        template<class H>
        constexpr H MaxEnum(H head) {
            return head;
        }

        template<class H, class N, class... T>
        constexpr H MaxEnum(H head, N next, T... tail) {
            return (UnderlyingValue)(next) < (UnderlyingValue)(head)
                   ? (MaxEnum)(head, tail...)
                   : (MaxEnum)(next, tail...);
        }

        template<class... Profs>
        struct CombineProfilesImpl {
            static constexpr default_constructible
                    default_constructible_support =  // NOLINT
                    (MaxEnum)(PropertiesOfT<Profs>::default_constructible_support...);

            static constexpr move_constructible move_constructible_support =  // NOLINT
                    (MaxEnum)(PropertiesOfT<Profs>::move_constructible_support...);

            static constexpr copy_constructible copy_constructible_support =  // NOLINT
                    (MaxEnum)(PropertiesOfT<Profs>::copy_constructible_support...);

            static constexpr move_assignable move_assignable_support =  // NOLINT
                    (MaxEnum)(PropertiesOfT<Profs>::move_assignable_support...);

            static constexpr copy_assignable copy_assignable_support =  // NOLINT
                    (MaxEnum)(PropertiesOfT<Profs>::copy_assignable_support...);

            static constexpr destructible destructible_support =  // NOLINT
                    (MaxEnum)(PropertiesOfT<Profs>::destructible_support...);

            static constexpr equality_comparable equality_comparable_support =  // NOLINT
                    (MaxEnum)(PropertiesOfT<Profs>::equality_comparable_support...);

            static constexpr inequality_comparable
                    inequality_comparable_support =  // NOLINT
                    (MaxEnum)(PropertiesOfT<Profs>::inequality_comparable_support...);

            static constexpr less_than_comparable
                    less_than_comparable_support =  // NOLINT
                    (MaxEnum)(PropertiesOfT<Profs>::less_than_comparable_support...);

            static constexpr less_equal_comparable
                    less_equal_comparable_support =  // NOLINT
                    (MaxEnum)(PropertiesOfT<Profs>::less_equal_comparable_support...);

            static constexpr greater_equal_comparable
                    greater_equal_comparable_support =  // NOLINT
                    (MaxEnum)(PropertiesOfT<Profs>::greater_equal_comparable_support...);

            static constexpr greater_than_comparable
                    greater_than_comparable_support =  // NOLINT
                    (MaxEnum)(PropertiesOfT<Profs>::greater_than_comparable_support...);

            static constexpr swappable swappable_support =  // NOLINT
                    (MaxEnum)(PropertiesOfT<Profs>::swappable_support...);

            static constexpr hashable hashable_support =  // NOLINT
                    (MaxEnum)(PropertiesOfT<Profs>::hashable_support...);

            using properties = ConformanceProfile<
                    default_constructible_support, move_constructible_support,
                    copy_constructible_support, move_assignable_support,
                    copy_assignable_support, destructible_support,
                    equality_comparable_support, inequality_comparable_support,
                    less_than_comparable_support, less_equal_comparable_support,
                    greater_equal_comparable_support, greater_than_comparable_support,
                    swappable_support, hashable_support>;
        };

// NOTE: We use this as opposed to a direct alias of CombineProfilesImpl so that
// when named aliases of CombineProfiles are created (such as in
// conformance_aliases.h), we only pay for the combination algorithm on the
// profiles that are actually used.
        template<class... Profs>
        struct CombineProfiles {
            using profile_alias_of = CombineProfilesImpl<Profs...>;
        };

        template<>
        struct CombineProfiles<> {
            using properties = ConformanceProfile<>;
        };

        template<class Profile, class Tag>
        struct StrongProfileTypedef {
            using properties = PropertiesOfT<Profile>;
        };

        template<class T, class /*Enabler*/ = void>
        struct IsProfileImpl : std::false_type {
        };

        template<class T>
        struct IsProfileImpl<T, abel::void_t<PropertiesOfT<T>>> : std::true_type {
        };

        template<class T>
        struct IsProfile : IsProfileImpl<T>::type {
        };

    }  // namespace types_internal

}  // namespace abel

#endif  // ABEL_ASL_INTERNAL_CONFORMANCE_PROFILE_H_
