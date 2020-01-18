//
// Helper class to perform the Empty Base Optimization.
// Ts can contain classes and non-classes, empty or not. For the ones that
// are empty classes, we perform the optimization. If all types in Ts are empty
// classes, then CompressedTuple<Ts...> is itself an empty class.
//
// To access the members, use member get<N>() function.
//
// Eg:
//   abel::container_internal::CompressedTuple<int, T1, T2, T3> value(7, t1, t2,
//                                                                    t3);
//   assert(value.get<0>() == 7);
//   T1& t1 = value.get<1>();
//   const T2& t2 = value.get<2>();
//   ...
//
// https://en.cppreference.com/w/cpp/language/ebo

#ifndef ABEL_CONTAINER_INTERNAL_COMPRESSED_TUPLE_H_
#define ABEL_CONTAINER_INTERNAL_COMPRESSED_TUPLE_H_

#include <initializer_list>
#include <tuple>
#include <type_traits>
#include <utility>

#include <abel/utility/utility.h>

#if defined(_MSC_VER) && !defined(__NVCC__)
// We need to mark these classes with this declspec to ensure that
// CompressedTuple happens.
#define ABEL_INTERNAL_COMPRESSED_TUPLE_DECLSPEC __declspec(empty_bases)
#else
#define ABEL_INTERNAL_COMPRESSED_TUPLE_DECLSPEC
#endif

namespace abel {
ABEL_NAMESPACE_BEGIN
namespace container_internal {

template <typename... Ts>
class CompressedTuple;

namespace internal_compressed_tuple {

template <typename D, size_t I>
struct Elem;
template <typename... B, size_t I>
struct Elem<CompressedTuple<B...>, I>
    : std::tuple_element<I, std::tuple<B...>> {};
template <typename D, size_t I>
using ElemT = typename Elem<D, I>::type;

// Use the __is_final intrinsic if available. Where it's not available, classes
// declared with the 'final' specifier cannot be used as CompressedTuple
// elements.
// TODO(sbenza): Replace this with std::is_final in C++14.
template <typename T>
constexpr bool IsFinal() {
#if defined(__clang__) || defined(__GNUC__)
  return __is_final(T);
#else
  return false;
#endif
}

// We can't use EBCO on other CompressedTuples because that would mean that we
// derive from multiple Storage<> instantiations with the same I parameter,
// and potentially from multiple identical Storage<> instantiations.  So anytime
// we use type inheritance rather than encapsulation, we mark
// CompressedTupleImpl, to make this easy to detect.
struct uses_inheritance {};

template <typename T>
constexpr bool ShouldUseBase() {
  return std::is_class<T>::value && std::is_empty<T>::value && !IsFinal<T>() &&
         !std::is_base_of<uses_inheritance, T>::value;
}

// The storage class provides two specializations:
//  - For empty classes, it stores T as a base class.
//  - For everything else, it stores T as a member.
template <typename T, size_t I,
#if defined(_MSC_VER)
          bool UseBase =
              ShouldUseBase<typename std::enable_if<true, T>::type>()>
#else
          bool UseBase = ShouldUseBase<T>()>
#endif
struct Storage {
  T value;
  constexpr Storage() = default;
  template <typename V>
  explicit constexpr Storage(abel::in_place_t, V&& v)
      : value(abel::forward<V>(v)) {}
  constexpr const T& get() const& { return value; }
  T& get() & { return value; }
  constexpr const T&& get() const&& { return abel::move(*this).value; }
  T&& get() && { return std::move(*this).value; }
};

template <typename T, size_t I>
struct ABEL_INTERNAL_COMPRESSED_TUPLE_DECLSPEC Storage<T, I, true> : T {
  constexpr Storage() = default;

  template <typename V>
  explicit constexpr Storage(abel::in_place_t, V&& v)
      : T(abel::forward<V>(v)) {}

  constexpr const T& get() const& { return *this; }
  T& get() & { return *this; }
  constexpr const T&& get() const&& { return abel::move(*this); }
  T&& get() && { return std::move(*this); }
};

template <typename D, typename I, bool ShouldAnyUseBase>
struct ABEL_INTERNAL_COMPRESSED_TUPLE_DECLSPEC CompressedTupleImpl;

template <typename... Ts, size_t... I, bool ShouldAnyUseBase>
struct ABEL_INTERNAL_COMPRESSED_TUPLE_DECLSPEC CompressedTupleImpl<
    CompressedTuple<Ts...>, abel::index_sequence<I...>, ShouldAnyUseBase>
    // We use the dummy identity function through std::integral_constant to
    // convince MSVC of accepting and expanding I in that context. Without it
    // you would get:
    //   error C3548: 'I': parameter pack cannot be used in this context
    : uses_inheritance,
      Storage<Ts, std::integral_constant<size_t, I>::value>... {
  constexpr CompressedTupleImpl() = default;
  template <typename... Vs>
  explicit constexpr CompressedTupleImpl(abel::in_place_t, Vs&&... args)
      : Storage<Ts, I>(abel::in_place, abel::forward<Vs>(args))... {}
  friend CompressedTuple<Ts...>;
};

template <typename... Ts, size_t... I>
struct ABEL_INTERNAL_COMPRESSED_TUPLE_DECLSPEC CompressedTupleImpl<
    CompressedTuple<Ts...>, abel::index_sequence<I...>, false>
    // We use the dummy identity function as above...
    : Storage<Ts, std::integral_constant<size_t, I>::value, false>... {
  constexpr CompressedTupleImpl() = default;
  template <typename... Vs>
  explicit constexpr CompressedTupleImpl(abel::in_place_t, Vs&&... args)
      : Storage<Ts, I, false>(abel::in_place, abel::forward<Vs>(args))... {}
  friend CompressedTuple<Ts...>;
};

std::false_type Or(std::initializer_list<std::false_type>);
std::true_type Or(std::initializer_list<bool>);

// MSVC requires this to be done separately rather than within the declaration
// of CompressedTuple below.
template <typename... Ts>
constexpr bool ShouldAnyUseBase() {
  return decltype(
      Or({std::integral_constant<bool, ShouldUseBase<Ts>()>()...})){};
}

template <typename T, typename V>
using TupleMoveConstructible = typename std::conditional<
      std::is_reference<T>::value, std::is_convertible<V, T>,
      std::is_constructible<T, V&&>>::type;

}  // namespace internal_compressed_tuple

// Helper class to perform the Empty Base Class Optimization.
// Ts can contain classes and non-classes, empty or not. For the ones that
// are empty classes, we perform the CompressedTuple. If all types in Ts are
// empty classes, then CompressedTuple<Ts...> is itself an empty class.  (This
// does not apply when one or more of those empty classes is itself an empty
// CompressedTuple.)
//
// To access the members, use member .get<N>() function.
//
// Eg:
//   abel::container_internal::CompressedTuple<int, T1, T2, T3> value(7, t1, t2,
//                                                                    t3);
//   assert(value.get<0>() == 7);
//   T1& t1 = value.get<1>();
//   const T2& t2 = value.get<2>();
//   ...
//
// https://en.cppreference.com/w/cpp/language/ebo
template <typename... Ts>
class ABEL_INTERNAL_COMPRESSED_TUPLE_DECLSPEC CompressedTuple
    : private internal_compressed_tuple::CompressedTupleImpl<
          CompressedTuple<Ts...>, abel::index_sequence_for<Ts...>,
          internal_compressed_tuple::ShouldAnyUseBase<Ts...>()> {
 private:
  template <int I>
  using ElemT = internal_compressed_tuple::ElemT<CompressedTuple, I>;

  template <int I>
  using StorageT = internal_compressed_tuple::Storage<ElemT<I>, I>;

 public:
  // There seems to be a bug in MSVC dealing in which using '=default' here will
  // cause the compiler to ignore the body of other constructors. The work-
  // around is to explicitly implement the default constructor.
#if defined(_MSC_VER)
  constexpr CompressedTuple() : CompressedTuple::CompressedTupleImpl() {}
#else
  constexpr CompressedTuple() = default;
#endif
  explicit constexpr CompressedTuple(const Ts&... base)
      : CompressedTuple::CompressedTupleImpl(abel::in_place, base...) {}

  template <typename... Vs,
            abel::enable_if_t<
                abel::conjunction<
                    // Ensure we are not hiding default copy/move constructors.
                    abel::negation<std::is_same<void(CompressedTuple),
                                                void(abel::decay_t<Vs>...)>>,
                    internal_compressed_tuple::TupleMoveConstructible<
                        Ts, Vs&&>...>::value,
                bool> = true>
  explicit constexpr CompressedTuple(Vs&&... base)
      : CompressedTuple::CompressedTupleImpl(abel::in_place,
                                             abel::forward<Vs>(base)...) {}

  template <int I>
  ElemT<I>& get() & {
    return internal_compressed_tuple::Storage<ElemT<I>, I>::get();
  }

  template <int I>
  constexpr const ElemT<I>& get() const& {
    return StorageT<I>::get();
  }

  template <int I>
  ElemT<I>&& get() && {
    return std::move(*this).StorageT<I>::get();
  }

  template <int I>
  constexpr const ElemT<I>&& get() const&& {
    return abel::move(*this).StorageT<I>::get();
  }
};

// Explicit specialization for a zero-element tuple
// (needed to avoid ambiguous overloads for the default constructor).
template <>
class ABEL_INTERNAL_COMPRESSED_TUPLE_DECLSPEC CompressedTuple<> {};

}  // namespace container_internal
ABEL_NAMESPACE_END
}  // namespace abel

#undef ABEL_INTERNAL_COMPRESSED_TUPLE_DECLSPEC

#endif  // ABEL_CONTAINER_INTERNAL_COMPRESSED_TUPLE_H_
