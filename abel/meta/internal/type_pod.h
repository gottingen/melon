// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com


#ifndef ABEL_META_INTERNAL_TYPE_POD_H_
#define ABEL_META_INTERNAL_TYPE_POD_H_

#include "abel/meta/internal/config.h"
#include "abel/meta/internal/type_transformation.h"

namespace abel {

template<typename A, typename B>
class is_widening_convertible {
    // As long as there are enough bits in the exact part of a number:
    // - unsigned can fit in float, signed, unsigned
    // - signed can fit in float, signed
    // - float can fit in float
    // So we define rank to be:
    // - rank(float) -> 2
    // - rank(signed) -> 1
    // - rank(unsigned) -> 0
    template<class T>
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

// Defined and documented later on in this file.
template<typename T>
struct is_trivially_destructible;

// Defined and documented later on in this file.
template<typename T>
struct is_trivially_move_assignable;

namespace asl_internal {

// Silence MSVC warnings about the destructor being defined as deleted.
#if defined(_MSC_VER) && !defined(__GNUC__)
#pragma warning(push)
#pragma warning(disable : 4624)
#endif  // defined(_MSC_VER) && !defined(__GNUC__)

template<class T>
union SingleMemberUnion {
    T t;
};

// Restore the state of the destructor warning that was silenced above.
#if defined(_MSC_VER) && !defined(__GNUC__)
#pragma warning(pop)
#endif  // defined(_MSC_VER) && !defined(__GNUC__)

template<class T>
struct IsTriviallyMoveConstructibleObject
        : std::integral_constant<
                bool, std::is_move_constructible<
                        asl_internal::SingleMemberUnion<T>>::value &&
                      abel::is_trivially_destructible<T>::value> {
};

template<class T>
struct IsTriviallyCopyConstructibleObject
        : std::integral_constant<
                bool, std::is_copy_constructible<
                        asl_internal::SingleMemberUnion<T>>::value &&
                      abel::is_trivially_destructible<T>::value> {
};

template<class T>
struct IsTriviallyMoveAssignableReference : std::false_type {
};

template<class T>
struct IsTriviallyMoveAssignableReference<T &>
        : abel::is_trivially_move_assignable<T>::type {
};

template<class T>
struct IsTriviallyMoveAssignableReference<T &&>
        : abel::is_trivially_move_assignable<T>::type {
};




template<typename T>
using IsCopyAssignableImpl =
decltype(std::declval<T &>() = std::declval<const T &>());

template<typename T>
using IsMoveAssignableImpl = decltype(std::declval<T &>() = std::declval<T &&>());

}  // namespace asl_internal



// MSVC 19.20 has a regression that causes our workarounds to fail, but their
// std forms now appear to be compliant.
#if defined(_MSC_VER) && !defined(__clang__) && (_MSC_VER >= 1920)

template <typename T>
using is_copy_assignable = std::is_copy_assignable<T>;

template <typename T>
using is_move_assignable = std::is_move_assignable<T>;

#else

template<typename T>
struct is_copy_assignable : is_detected<
        asl_internal::IsCopyAssignableImpl, T> {
};

template<typename T>
struct is_move_assignable : is_detected<
        asl_internal::IsMoveAssignableImpl, T> {
};

#endif

// is_trivially_destructible()
//
// Determines whether the passed type `T` is trivially destructible.
//
// This metafunction is designed to be a drop-in replacement for the C++11
// `std::is_trivially_destructible()` metafunction for platforms that have
// incomplete C++11 support (such as libstdc++ 4.x). On any platforms that do
// fully support C++11, we check whether this yields the same result as the std
// implementation.
//
// NOTE: the extensions (__has_trivial_xxx) are implemented in gcc (version >=
// 4.3) and clang. Since we are supporting libstdc++ > 4.7, they should always
// be present. These  extensions are documented at
// https://gcc.gnu.org/onlinedocs/gcc/Type-Traits.html#Type-Traits.
template<typename T>
struct is_trivially_destructible
        : std::integral_constant<bool, __has_trivial_destructor(T) &&
                                       std::is_destructible<T>::value> {
#ifdef ABEL_HAVE_STD_IS_TRIVIALLY_DESTRUCTIBLE
  private:
    static constexpr bool compliant = std::is_trivially_destructible<T>::value ==
                                      is_trivially_destructible::value;
    static_assert(compliant || std::is_trivially_destructible<T>::value,
                  "Not compliant with std::is_trivially_destructible; "
                  "Standard: false, Implementation: true");
    static_assert(compliant || !std::is_trivially_destructible<T>::value,
                  "Not compliant with std::is_trivially_destructible; "
                  "Standard: true, Implementation: false");
#endif  // ABEL_HAVE_STD_IS_TRIVIALLY_DESTRUCTIBLE
};

// is_trivially_default_constructible()
//
// Determines whether the passed type `T` is trivially default constructible.
//
// This metafunction is designed to be a drop-in replacement for the C++11
// `std::is_trivially_default_constructible()` metafunction for platforms that
// have incomplete C++11 support (such as libstdc++ 4.x). On any platforms that
// do fully support C++11, we check whether this yields the same result as the
// std implementation.
//
// NOTE: according to the C++ standard, Section: 20.15.4.3 [meta.unary.prop]
// "The predicate condition for a template specialization is_constructible<T,
// Args...> shall be satisfied if and only if the following variable
// definition would be well-formed for some invented variable t:
//
// T t(declval<Args>()...);
//
// is_trivially_constructible<T, Args...> additionally requires that the
// variable definition does not call any operation that is not trivial.
// For the purposes of this check, the call to std::declval is considered
// trivial."
//
// Notes from https://en.cppreference.com/w/cpp/types/is_constructible:
// In many implementations, is_nothrow_constructible also checks if the
// destructor throws because it is effectively noexcept(T(arg)). Same
// applies to is_trivially_constructible, which, in these implementations, also
// requires that the destructor is trivial.
// GCC bug 51452: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=51452
// LWG issue 2116: http://cplusplus.github.io/LWG/lwg-active.html#2116.
//
// "T obj();" need to be well-formed and not call any nontrivial operation.
// Nontrivially destructible types will cause the expression to be nontrivial.
template<typename T>
struct is_trivially_default_constructible
        : std::integral_constant<bool, __has_trivial_constructor(T) &&
                                       std::is_default_constructible<T>::value &&
                                       is_trivially_destructible<T>::value> {
#if defined(ABEL_HAVE_STD_IS_TRIVIALLY_CONSTRUCTIBLE) && \
    !defined(                                            \
        ABEL_META_INTERNAL_STD_CONSTRUCTION_TRAITS_DONT_CHECK_DESTRUCTION)
  private:
    static constexpr bool compliant =
            std::is_trivially_default_constructible<T>::value ==
            is_trivially_default_constructible::value;
    static_assert(compliant || std::is_trivially_default_constructible<T>::value,
                  "Not compliant with std::is_trivially_default_constructible; "
                  "Standard: false, Implementation: true");
    static_assert(compliant || !std::is_trivially_default_constructible<T>::value,
                  "Not compliant with std::is_trivially_default_constructible; "
                  "Standard: true, Implementation: false");
#endif  // ABEL_HAVE_STD_IS_TRIVIALLY_CONSTRUCTIBLE
};

// is_trivially_move_constructible()
//
// Determines whether the passed type `T` is trivially move constructible.
//
// This metafunction is designed to be a drop-in replacement for the C++11
// `std::is_trivially_move_constructible()` metafunction for platforms that have
// incomplete C++11 support (such as libstdc++ 4.x). On any platforms that do
// fully support C++11, we check whether this yields the same result as the std
// implementation.
//
// NOTE: `T obj(declval<T>());` needs to be well-formed and not call any
// nontrivial operation.  Nontrivially destructible types will cause the
// expression to be nontrivial.
template<typename T>
struct is_trivially_move_constructible
        : std::conditional<
                std::is_object<T>::value && !std::is_array<T>::value,
                asl_internal::IsTriviallyMoveConstructibleObject<T>,
                std::is_reference<T>>
          ::type::type {
#if defined(ABEL_HAVE_STD_IS_TRIVIALLY_CONSTRUCTIBLE) && \
    !defined(                                            \
        ABEL_META_INTERNAL_STD_CONSTRUCTION_TRAITS_DONT_CHECK_DESTRUCTION)
  private:
    static constexpr bool compliant =
            std::is_trivially_move_constructible<T>::value ==
            is_trivially_move_constructible::value;
    static_assert(compliant || std::is_trivially_move_constructible<T>::value,
                  "Not compliant with std::is_trivially_move_constructible; "
                  "Standard: false, Implementation: true");
    static_assert(compliant || !std::is_trivially_move_constructible<T>::value,
                  "Not compliant with std::is_trivially_move_constructible; "
                  "Standard: true, Implementation: false");
#endif  // ABEL_HAVE_STD_IS_TRIVIALLY_CONSTRUCTIBLE
};

// is_trivially_copy_constructible()
//
// Determines whether the passed type `T` is trivially copy constructible.
//
// This metafunction is designed to be a drop-in replacement for the C++11
// `std::is_trivially_copy_constructible()` metafunction for platforms that have
// incomplete C++11 support (such as libstdc++ 4.x). On any platforms that do
// fully support C++11, we check whether this yields the same result as the std
// implementation.
//
// NOTE: `T obj(declval<const T&>());` needs to be well-formed and not call any
// nontrivial operation.  Nontrivially destructible types will cause the
// expression to be nontrivial.
template<typename T>
struct is_trivially_copy_constructible
        : std::conditional<
                std::is_object<T>::value && !std::is_array<T>::value,
                asl_internal::IsTriviallyCopyConstructibleObject<T>,
                std::is_lvalue_reference<T>>
          ::type::type {
#if defined(ABEL_HAVE_STD_IS_TRIVIALLY_CONSTRUCTIBLE) && \
    !defined(                                            \
        ABEL_META_INTERNAL_STD_CONSTRUCTION_TRAITS_DONT_CHECK_DESTRUCTION)
  private:
    static constexpr bool compliant =
            std::is_trivially_copy_constructible<T>::value ==
            is_trivially_copy_constructible::value;
    static_assert(compliant || std::is_trivially_copy_constructible<T>::value,
                  "Not compliant with std::is_trivially_copy_constructible; "
                  "Standard: false, Implementation: true");
    static_assert(compliant || !std::is_trivially_copy_constructible<T>::value,
                  "Not compliant with std::is_trivially_copy_constructible; "
                  "Standard: true, Implementation: false");
#endif  // ABEL_HAVE_STD_IS_TRIVIALLY_CONSTRUCTIBLE
};

// is_trivially_move_assignable()
//
// Determines whether the passed type `T` is trivially move assignable.
//
// This metafunction is designed to be a drop-in replacement for the C++11
// `std::is_trivially_move_assignable()` metafunction for platforms that have
// incomplete C++11 support (such as libstdc++ 4.x). On any platforms that do
// fully support C++11, we check whether this yields the same result as the std
// implementation.
//
// NOTE: `is_assignable<T, U>::value` is `true` if the expression
// `declval<T>() = declval<U>()` is well-formed when treated as an unevaluated
// operand. `is_trivially_assignable<T, U>` requires the assignment to call no
// operation that is not trivial. `is_trivially_copy_assignable<T>` is simply
// `is_trivially_assignable<T&, T>`.
template<typename T>
struct is_trivially_move_assignable
        : std::conditional<
                std::is_object<T>::value && !std::is_array<T>::value &&
                std::is_move_assignable<T>::value,
                std::is_move_assignable<asl_internal::SingleMemberUnion<T>>,
                asl_internal::IsTriviallyMoveAssignableReference<T>>
          ::type::
          type {
#ifdef ABEL_HAVE_STD_IS_TRIVIALLY_ASSIGNABLE
  private:
    static constexpr bool compliant =
            std::is_trivially_move_assignable<T>::value ==
            is_trivially_move_assignable::value;
    static_assert(compliant || std::is_trivially_move_assignable<T>::value,
                  "Not compliant with std::is_trivially_move_assignable; "
                  "Standard: false, Implementation: true");
    static_assert(compliant || !std::is_trivially_move_assignable<T>::value,
                  "Not compliant with std::is_trivially_move_assignable; "
                  "Standard: true, Implementation: false");
#endif  // ABEL_HAVE_STD_IS_TRIVIALLY_ASSIGNABLE
};

// is_trivially_copy_assignable()
//
// Determines whether the passed type `T` is trivially copy assignable.
//
// This metafunction is designed to be a drop-in replacement for the C++11
// `std::is_trivially_copy_assignable()` metafunction for platforms that have
// incomplete C++11 support (such as libstdc++ 4.x). On any platforms that do
// fully support C++11, we check whether this yields the same result as the std
// implementation.
//
// NOTE: `is_assignable<T, U>::value` is `true` if the expression
// `declval<T>() = declval<U>()` is well-formed when treated as an unevaluated
// operand. `is_trivially_assignable<T, U>` requires the assignment to call no
// operation that is not trivial. `is_trivially_copy_assignable<T>` is simply
// `is_trivially_assignable<T&, const T&>`.
template<typename T>
struct is_trivially_copy_assignable
        : std::integral_constant<
                bool, __has_trivial_assign(typename std::remove_reference<T>::type) &&
                      abel::is_copy_assignable<T>::value> {
#ifdef ABEL_HAVE_STD_IS_TRIVIALLY_ASSIGNABLE
  private:
    static constexpr bool compliant =
            std::is_trivially_copy_assignable<T>::value ==
            is_trivially_copy_assignable::value;
    static_assert(compliant || std::is_trivially_copy_assignable<T>::value,
                  "Not compliant with std::is_trivially_copy_assignable; "
                  "Standard: false, Implementation: true");
    static_assert(compliant || !std::is_trivially_copy_assignable<T>::value,
                  "Not compliant with std::is_trivially_copy_assignable; "
                  "Standard: true, Implementation: false");
#endif  // ABEL_HAVE_STD_IS_TRIVIALLY_ASSIGNABLE
};

// is_trivially_copyable()
//
// Determines whether the passed type `T` is trivially copyable.
//
// This metafunction is designed to be a drop-in replacement for the C++11
// `std::is_trivially_copyable()` metafunction for platforms that have
// incomplete C++11 support (such as libstdc++ 4.x). We use the C++17 definition
// of TriviallyCopyable.
//
// NOTE: `is_trivially_copyable<T>::value` is `true` if all of T's copy/move
// constructors/assignment operators are trivial or deleted, T has at least
// one non-deleted copy/move constructor/assignment operator, and T is trivially
// destructible. Arrays of trivially copyable types are trivially copyable.
//
// We expose this metafunction only for internal use within abel.
namespace asl_internal {
template<typename T>
class is_trivially_copyable_impl {
    using ExtentsRemoved = typename std::remove_all_extents<T>::type;
    static constexpr bool kIsCopyOrMoveConstructible =
            std::is_copy_constructible<ExtentsRemoved>::value ||
            std::is_move_constructible<ExtentsRemoved>::value;
    static constexpr bool kIsCopyOrMoveAssignable =
            abel::is_copy_assignable<ExtentsRemoved>::value ||
            abel::is_move_assignable<ExtentsRemoved>::value;

  public:
    static constexpr bool kValue =
            (__has_trivial_copy(ExtentsRemoved) || !kIsCopyOrMoveConstructible) &&
            (__has_trivial_assign(ExtentsRemoved) || !kIsCopyOrMoveAssignable) &&
            (kIsCopyOrMoveConstructible || kIsCopyOrMoveAssignable) &&
            is_trivially_destructible<ExtentsRemoved>::value &&
            // We need to check for this explicitly because otherwise we'll say
            // references are trivial copyable when compiled by MSVC.
            !std::is_reference<ExtentsRemoved>::value;
};
}
template<typename T>
struct is_trivially_copyable
        : std::integral_constant<
                bool, asl_internal::is_trivially_copyable_impl<T>::kValue> {
};

}  // namespace abel

#endif  // ABEL_META_INTERNAL_TYPE_POD_H_

