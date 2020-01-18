//
// -----------------------------------------------------------------------------
// compare.h
// -----------------------------------------------------------------------------
//
// This header file defines the `abel::weak_equality`, `abel::strong_equality`,
// `abel::partial_ordering`, `abel::weak_ordering`, and `abel::strong_ordering`
// types for storing the results of three way comparisons.
//
// Example:
//   abel::weak_ordering compare(const std::string& a, const std::string& b);
//
// These are C++11 compatible versions of the C++20 corresponding types
// (`std::weak_equality`, etc.) and are designed to be drop-in replacements
// for code compliant with C++20.

#ifndef ABEL_TYPES_COMPARE_H_
#define ABEL_TYPES_COMPARE_H_

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <type_traits>

#include <abel/base/profile.h>
#include <abel/meta/type_traits.h>

namespace abel {
ABEL_NAMESPACE_BEGIN
namespace compare_internal {

using value_type = int8_t;

template <typename T>
struct fail {
  static_assert(sizeof(T) < 0, "Only literal `0` is allowed.");
};

// We need the NullPtrT template to avoid triggering the modernize-use-nullptr
// ClangTidy warning in user code.
template <typename NullPtrT = std::nullptr_t>
struct only_literal_zero {
  constexpr only_literal_zero(NullPtrT) noexcept {}  // NOLINT

  // Fails compilation when `nullptr` or integral type arguments other than
  // `int` are passed. This constructor doesn't accept `int` because literal `0`
  // has type `int`. Literal `0` arguments will be implicitly converted to
  // `std::nullptr_t` and accepted by the above constructor, while other `int`
  // arguments will fail to be converted and cause compilation failure.
  template <
      typename T,
      typename = typename std::enable_if<
          std::is_same<T, std::nullptr_t>::value ||
          (std::is_integral<T>::value && !std::is_same<T, int>::value)>::type,
      typename = typename fail<T>::type>
  only_literal_zero(T);  // NOLINT
};

enum class eq : value_type {
  equal = 0,
  equivalent = equal,
  nonequal = 1,
  nonequivalent = nonequal,
};

enum class ord : value_type { less = -1, greater = 1 };

enum class ncmp : value_type { unordered = -127 };

// Define macros to allow for creation or emulation of C++17 inline variables
// based on whether the feature is supported. Note: we can't use
// ABEL_INTERNAL_INLINE_CONSTEXPR here because the variables here are of
// incomplete types so they need to be defined after the types are complete.
#ifdef __cpp_inline_variables

#define ABEL_COMPARE_INLINE_BASECLASS_DECL(name)

#define ABEL_COMPARE_INLINE_SUBCLASS_DECL(type, name) \
  static const type name

#define ABEL_COMPARE_INLINE_INIT(type, name, init) \
  ABEL_FORCE_INLINE constexpr type type::name(init)

#else  // __cpp_inline_variables

#define ABEL_COMPARE_INLINE_BASECLASS_DECL(name) \
  ABEL_CONST_INIT static const T name

#define ABEL_COMPARE_INLINE_SUBCLASS_DECL(type, name)

#define ABEL_COMPARE_INLINE_INIT(type, name, init) \
  template <typename T>                            \
  const T compare_internal::type##_base<T>::name(init)

#endif  // __cpp_inline_variables

// These template base classes allow for defining the values of the constants
// in the header file (for performance) without using inline variables (which
// aren't available in C++11).
template <typename T>
struct weak_equality_base {
  ABEL_COMPARE_INLINE_BASECLASS_DECL(equivalent);
  ABEL_COMPARE_INLINE_BASECLASS_DECL(nonequivalent);
};

template <typename T>
struct strong_equality_base {
  ABEL_COMPARE_INLINE_BASECLASS_DECL(equal);
  ABEL_COMPARE_INLINE_BASECLASS_DECL(nonequal);
  ABEL_COMPARE_INLINE_BASECLASS_DECL(equivalent);
  ABEL_COMPARE_INLINE_BASECLASS_DECL(nonequivalent);
};

template <typename T>
struct partial_ordering_base {
  ABEL_COMPARE_INLINE_BASECLASS_DECL(less);
  ABEL_COMPARE_INLINE_BASECLASS_DECL(equivalent);
  ABEL_COMPARE_INLINE_BASECLASS_DECL(greater);
  ABEL_COMPARE_INLINE_BASECLASS_DECL(unordered);
};

template <typename T>
struct weak_ordering_base {
  ABEL_COMPARE_INLINE_BASECLASS_DECL(less);
  ABEL_COMPARE_INLINE_BASECLASS_DECL(equivalent);
  ABEL_COMPARE_INLINE_BASECLASS_DECL(greater);
};

template <typename T>
struct strong_ordering_base {
  ABEL_COMPARE_INLINE_BASECLASS_DECL(less);
  ABEL_COMPARE_INLINE_BASECLASS_DECL(equal);
  ABEL_COMPARE_INLINE_BASECLASS_DECL(equivalent);
  ABEL_COMPARE_INLINE_BASECLASS_DECL(greater);
};

}  // namespace compare_internal

class weak_equality
    : public compare_internal::weak_equality_base<weak_equality> {
  explicit constexpr weak_equality(compare_internal::eq v) noexcept
      : value_(static_cast<compare_internal::value_type>(v)) {}
  friend struct compare_internal::weak_equality_base<weak_equality>;

 public:
  ABEL_COMPARE_INLINE_SUBCLASS_DECL(weak_equality, equivalent);
  ABEL_COMPARE_INLINE_SUBCLASS_DECL(weak_equality, nonequivalent);

  // Comparisons
  friend constexpr bool operator==(
      weak_equality v, compare_internal::only_literal_zero<>) noexcept {
    return v.value_ == 0;
  }
  friend constexpr bool operator!=(
      weak_equality v, compare_internal::only_literal_zero<>) noexcept {
    return v.value_ != 0;
  }
  friend constexpr bool operator==(compare_internal::only_literal_zero<>,
                                   weak_equality v) noexcept {
    return 0 == v.value_;
  }
  friend constexpr bool operator!=(compare_internal::only_literal_zero<>,
                                   weak_equality v) noexcept {
    return 0 != v.value_;
  }

 private:
  compare_internal::value_type value_;
};
ABEL_COMPARE_INLINE_INIT(weak_equality, equivalent,
                         compare_internal::eq::equivalent);
ABEL_COMPARE_INLINE_INIT(weak_equality, nonequivalent,
                         compare_internal::eq::nonequivalent);

class strong_equality
    : public compare_internal::strong_equality_base<strong_equality> {
  explicit constexpr strong_equality(compare_internal::eq v) noexcept
      : value_(static_cast<compare_internal::value_type>(v)) {}
  friend struct compare_internal::strong_equality_base<strong_equality>;

 public:
  ABEL_COMPARE_INLINE_SUBCLASS_DECL(strong_equality, equal);
  ABEL_COMPARE_INLINE_SUBCLASS_DECL(strong_equality, nonequal);
  ABEL_COMPARE_INLINE_SUBCLASS_DECL(strong_equality, equivalent);
  ABEL_COMPARE_INLINE_SUBCLASS_DECL(strong_equality, nonequivalent);

  // Conversion
  constexpr operator weak_equality() const noexcept {  // NOLINT
    return value_ == 0 ? weak_equality::equivalent
                       : weak_equality::nonequivalent;
  }
  // Comparisons
  friend constexpr bool operator==(
      strong_equality v, compare_internal::only_literal_zero<>) noexcept {
    return v.value_ == 0;
  }
  friend constexpr bool operator!=(
      strong_equality v, compare_internal::only_literal_zero<>) noexcept {
    return v.value_ != 0;
  }
  friend constexpr bool operator==(compare_internal::only_literal_zero<>,
                                   strong_equality v) noexcept {
    return 0 == v.value_;
  }
  friend constexpr bool operator!=(compare_internal::only_literal_zero<>,
                                   strong_equality v) noexcept {
    return 0 != v.value_;
  }

 private:
  compare_internal::value_type value_;
};
ABEL_COMPARE_INLINE_INIT(strong_equality, equal, compare_internal::eq::equal);
ABEL_COMPARE_INLINE_INIT(strong_equality, nonequal,
                         compare_internal::eq::nonequal);
ABEL_COMPARE_INLINE_INIT(strong_equality, equivalent,
                         compare_internal::eq::equivalent);
ABEL_COMPARE_INLINE_INIT(strong_equality, nonequivalent,
                         compare_internal::eq::nonequivalent);

class partial_ordering
    : public compare_internal::partial_ordering_base<partial_ordering> {
  explicit constexpr partial_ordering(compare_internal::eq v) noexcept
      : value_(static_cast<compare_internal::value_type>(v)) {}
  explicit constexpr partial_ordering(compare_internal::ord v) noexcept
      : value_(static_cast<compare_internal::value_type>(v)) {}
  explicit constexpr partial_ordering(compare_internal::ncmp v) noexcept
      : value_(static_cast<compare_internal::value_type>(v)) {}
  friend struct compare_internal::partial_ordering_base<partial_ordering>;

  constexpr bool is_ordered() const noexcept {
    return value_ !=
           compare_internal::value_type(compare_internal::ncmp::unordered);
  }

 public:
  ABEL_COMPARE_INLINE_SUBCLASS_DECL(partial_ordering, less);
  ABEL_COMPARE_INLINE_SUBCLASS_DECL(partial_ordering, equivalent);
  ABEL_COMPARE_INLINE_SUBCLASS_DECL(partial_ordering, greater);
  ABEL_COMPARE_INLINE_SUBCLASS_DECL(partial_ordering, unordered);

  // Conversion
  constexpr operator weak_equality() const noexcept {  // NOLINT
    return value_ == 0 ? weak_equality::equivalent
                       : weak_equality::nonequivalent;
  }
  // Comparisons
  friend constexpr bool operator==(
      partial_ordering v, compare_internal::only_literal_zero<>) noexcept {
    return v.is_ordered() && v.value_ == 0;
  }
  friend constexpr bool operator!=(
      partial_ordering v, compare_internal::only_literal_zero<>) noexcept {
    return !v.is_ordered() || v.value_ != 0;
  }
  friend constexpr bool operator<(
      partial_ordering v, compare_internal::only_literal_zero<>) noexcept {
    return v.is_ordered() && v.value_ < 0;
  }
  friend constexpr bool operator<=(
      partial_ordering v, compare_internal::only_literal_zero<>) noexcept {
    return v.is_ordered() && v.value_ <= 0;
  }
  friend constexpr bool operator>(
      partial_ordering v, compare_internal::only_literal_zero<>) noexcept {
    return v.is_ordered() && v.value_ > 0;
  }
  friend constexpr bool operator>=(
      partial_ordering v, compare_internal::only_literal_zero<>) noexcept {
    return v.is_ordered() && v.value_ >= 0;
  }
  friend constexpr bool operator==(compare_internal::only_literal_zero<>,
                                   partial_ordering v) noexcept {
    return v.is_ordered() && 0 == v.value_;
  }
  friend constexpr bool operator!=(compare_internal::only_literal_zero<>,
                                   partial_ordering v) noexcept {
    return !v.is_ordered() || 0 != v.value_;
  }
  friend constexpr bool operator<(compare_internal::only_literal_zero<>,
                                  partial_ordering v) noexcept {
    return v.is_ordered() && 0 < v.value_;
  }
  friend constexpr bool operator<=(compare_internal::only_literal_zero<>,
                                   partial_ordering v) noexcept {
    return v.is_ordered() && 0 <= v.value_;
  }
  friend constexpr bool operator>(compare_internal::only_literal_zero<>,
                                  partial_ordering v) noexcept {
    return v.is_ordered() && 0 > v.value_;
  }
  friend constexpr bool operator>=(compare_internal::only_literal_zero<>,
                                   partial_ordering v) noexcept {
    return v.is_ordered() && 0 >= v.value_;
  }

 private:
  compare_internal::value_type value_;
};
ABEL_COMPARE_INLINE_INIT(partial_ordering, less, compare_internal::ord::less);
ABEL_COMPARE_INLINE_INIT(partial_ordering, equivalent,
                         compare_internal::eq::equivalent);
ABEL_COMPARE_INLINE_INIT(partial_ordering, greater,
                         compare_internal::ord::greater);
ABEL_COMPARE_INLINE_INIT(partial_ordering, unordered,
                         compare_internal::ncmp::unordered);

class weak_ordering
    : public compare_internal::weak_ordering_base<weak_ordering> {
  explicit constexpr weak_ordering(compare_internal::eq v) noexcept
      : value_(static_cast<compare_internal::value_type>(v)) {}
  explicit constexpr weak_ordering(compare_internal::ord v) noexcept
      : value_(static_cast<compare_internal::value_type>(v)) {}
  friend struct compare_internal::weak_ordering_base<weak_ordering>;

 public:
  ABEL_COMPARE_INLINE_SUBCLASS_DECL(weak_ordering, less);
  ABEL_COMPARE_INLINE_SUBCLASS_DECL(weak_ordering, equivalent);
  ABEL_COMPARE_INLINE_SUBCLASS_DECL(weak_ordering, greater);

  // Conversions
  constexpr operator weak_equality() const noexcept {  // NOLINT
    return value_ == 0 ? weak_equality::equivalent
                       : weak_equality::nonequivalent;
  }
  constexpr operator partial_ordering() const noexcept {  // NOLINT
    return value_ == 0 ? partial_ordering::equivalent
                       : (value_ < 0 ? partial_ordering::less
                                     : partial_ordering::greater);
  }
  // Comparisons
  friend constexpr bool operator==(
      weak_ordering v, compare_internal::only_literal_zero<>) noexcept {
    return v.value_ == 0;
  }
  friend constexpr bool operator!=(
      weak_ordering v, compare_internal::only_literal_zero<>) noexcept {
    return v.value_ != 0;
  }
  friend constexpr bool operator<(
      weak_ordering v, compare_internal::only_literal_zero<>) noexcept {
    return v.value_ < 0;
  }
  friend constexpr bool operator<=(
      weak_ordering v, compare_internal::only_literal_zero<>) noexcept {
    return v.value_ <= 0;
  }
  friend constexpr bool operator>(
      weak_ordering v, compare_internal::only_literal_zero<>) noexcept {
    return v.value_ > 0;
  }
  friend constexpr bool operator>=(
      weak_ordering v, compare_internal::only_literal_zero<>) noexcept {
    return v.value_ >= 0;
  }
  friend constexpr bool operator==(compare_internal::only_literal_zero<>,
                                   weak_ordering v) noexcept {
    return 0 == v.value_;
  }
  friend constexpr bool operator!=(compare_internal::only_literal_zero<>,
                                   weak_ordering v) noexcept {
    return 0 != v.value_;
  }
  friend constexpr bool operator<(compare_internal::only_literal_zero<>,
                                  weak_ordering v) noexcept {
    return 0 < v.value_;
  }
  friend constexpr bool operator<=(compare_internal::only_literal_zero<>,
                                   weak_ordering v) noexcept {
    return 0 <= v.value_;
  }
  friend constexpr bool operator>(compare_internal::only_literal_zero<>,
                                  weak_ordering v) noexcept {
    return 0 > v.value_;
  }
  friend constexpr bool operator>=(compare_internal::only_literal_zero<>,
                                   weak_ordering v) noexcept {
    return 0 >= v.value_;
  }

 private:
  compare_internal::value_type value_;
};
ABEL_COMPARE_INLINE_INIT(weak_ordering, less, compare_internal::ord::less);
ABEL_COMPARE_INLINE_INIT(weak_ordering, equivalent,
                         compare_internal::eq::equivalent);
ABEL_COMPARE_INLINE_INIT(weak_ordering, greater,
                         compare_internal::ord::greater);

class strong_ordering
    : public compare_internal::strong_ordering_base<strong_ordering> {
  explicit constexpr strong_ordering(compare_internal::eq v) noexcept
      : value_(static_cast<compare_internal::value_type>(v)) {}
  explicit constexpr strong_ordering(compare_internal::ord v) noexcept
      : value_(static_cast<compare_internal::value_type>(v)) {}
  friend struct compare_internal::strong_ordering_base<strong_ordering>;

 public:
  ABEL_COMPARE_INLINE_SUBCLASS_DECL(strong_ordering, less);
  ABEL_COMPARE_INLINE_SUBCLASS_DECL(strong_ordering, equal);
  ABEL_COMPARE_INLINE_SUBCLASS_DECL(strong_ordering, equivalent);
  ABEL_COMPARE_INLINE_SUBCLASS_DECL(strong_ordering, greater);

  // Conversions
  constexpr operator weak_equality() const noexcept {  // NOLINT
    return value_ == 0 ? weak_equality::equivalent
                       : weak_equality::nonequivalent;
  }
  constexpr operator strong_equality() const noexcept {  // NOLINT
    return value_ == 0 ? strong_equality::equal : strong_equality::nonequal;
  }
  constexpr operator partial_ordering() const noexcept {  // NOLINT
    return value_ == 0 ? partial_ordering::equivalent
                       : (value_ < 0 ? partial_ordering::less
                                     : partial_ordering::greater);
  }
  constexpr operator weak_ordering() const noexcept {  // NOLINT
    return value_ == 0
               ? weak_ordering::equivalent
               : (value_ < 0 ? weak_ordering::less : weak_ordering::greater);
  }
  // Comparisons
  friend constexpr bool operator==(
      strong_ordering v, compare_internal::only_literal_zero<>) noexcept {
    return v.value_ == 0;
  }
  friend constexpr bool operator!=(
      strong_ordering v, compare_internal::only_literal_zero<>) noexcept {
    return v.value_ != 0;
  }
  friend constexpr bool operator<(
      strong_ordering v, compare_internal::only_literal_zero<>) noexcept {
    return v.value_ < 0;
  }
  friend constexpr bool operator<=(
      strong_ordering v, compare_internal::only_literal_zero<>) noexcept {
    return v.value_ <= 0;
  }
  friend constexpr bool operator>(
      strong_ordering v, compare_internal::only_literal_zero<>) noexcept {
    return v.value_ > 0;
  }
  friend constexpr bool operator>=(
      strong_ordering v, compare_internal::only_literal_zero<>) noexcept {
    return v.value_ >= 0;
  }
  friend constexpr bool operator==(compare_internal::only_literal_zero<>,
                                   strong_ordering v) noexcept {
    return 0 == v.value_;
  }
  friend constexpr bool operator!=(compare_internal::only_literal_zero<>,
                                   strong_ordering v) noexcept {
    return 0 != v.value_;
  }
  friend constexpr bool operator<(compare_internal::only_literal_zero<>,
                                  strong_ordering v) noexcept {
    return 0 < v.value_;
  }
  friend constexpr bool operator<=(compare_internal::only_literal_zero<>,
                                   strong_ordering v) noexcept {
    return 0 <= v.value_;
  }
  friend constexpr bool operator>(compare_internal::only_literal_zero<>,
                                  strong_ordering v) noexcept {
    return 0 > v.value_;
  }
  friend constexpr bool operator>=(compare_internal::only_literal_zero<>,
                                   strong_ordering v) noexcept {
    return 0 >= v.value_;
  }

 private:
  compare_internal::value_type value_;
};
ABEL_COMPARE_INLINE_INIT(strong_ordering, less, compare_internal::ord::less);
ABEL_COMPARE_INLINE_INIT(strong_ordering, equal, compare_internal::eq::equal);
ABEL_COMPARE_INLINE_INIT(strong_ordering, equivalent,
                         compare_internal::eq::equivalent);
ABEL_COMPARE_INLINE_INIT(strong_ordering, greater,
                         compare_internal::ord::greater);

#undef ABEL_COMPARE_INLINE_BASECLASS_DECL
#undef ABEL_COMPARE_INLINE_SUBCLASS_DECL
#undef ABEL_COMPARE_INLINE_INIT

namespace compare_internal {
// We also provide these comparator adapter functions for internal abel use.

// Helper functions to do a boolean comparison of two keys given a boolean
// or three-way comparator.
// SFINAE prevents implicit conversions to bool (such as from int).
template <typename Bool,
          abel::enable_if_t<std::is_same<bool, Bool>::value, int> = 0>
constexpr bool compare_result_as_less_than(const Bool r) { return r; }
constexpr bool compare_result_as_less_than(const abel::weak_ordering r) {
  return r < 0;
}

template <typename Compare, typename K, typename LK>
constexpr bool do_less_than_comparison(const Compare &compare, const K &x,
                                       const LK &y) {
  return compare_result_as_less_than(compare(x, y));
}

// Helper functions to do a three-way comparison of two keys given a boolean or
// three-way comparator.
// SFINAE prevents implicit conversions to int (such as from bool).
template <typename Int,
          abel::enable_if_t<std::is_same<int, Int>::value, int> = 0>
constexpr abel::weak_ordering compare_result_as_ordering(const Int c) {
  return c < 0 ? abel::weak_ordering::less
               : c == 0 ? abel::weak_ordering::equivalent
                        : abel::weak_ordering::greater;
}
constexpr abel::weak_ordering compare_result_as_ordering(
    const abel::weak_ordering c) {
  return c;
}

template <
    typename Compare, typename K, typename LK,
    abel::enable_if_t<!std::is_same<bool, abel::result_of_t<Compare(
                                              const K &, const LK &)>>::value,
                      int> = 0>
constexpr abel::weak_ordering do_three_way_comparison(const Compare &compare,
                                                      const K &x, const LK &y) {
  return compare_result_as_ordering(compare(x, y));
}
template <
    typename Compare, typename K, typename LK,
    abel::enable_if_t<std::is_same<bool, abel::result_of_t<Compare(
                                             const K &, const LK &)>>::value,
                      int> = 0>
constexpr abel::weak_ordering do_three_way_comparison(const Compare &compare,
                                                      const K &x, const LK &y) {
  return compare(x, y) ? abel::weak_ordering::less
                       : compare(y, x) ? abel::weak_ordering::greater
                                       : abel::weak_ordering::equivalent;
}

}  // namespace compare_internal
ABEL_NAMESPACE_END
}  // namespace abel

#endif  // ABEL_TYPES_COMPARE_H_
