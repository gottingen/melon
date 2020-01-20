//
// Generates random values for testing. Specialized only for the few types we
// care about.

#ifndef ABEL_CONTAINER_INTERNAL_HASH_GENERATOR_TESTING_H_
#define ABEL_CONTAINER_INTERNAL_HASH_GENERATOR_TESTING_H_

#include <stdint.h>

#include <algorithm>
#include <iosfwd>
#include <random>
#include <tuple>
#include <type_traits>
#include <utility>

#include <test/container/hash_policy_testing.h>
#include <abel/memory/memory.h>
#include <abel/meta/type_traits.h>
#include <abel/strings/string_view.h>

namespace abel {

namespace container_internal {
namespace hash_internal {
namespace generator_internal {

template <class Container, class = void>
struct IsMap : std::false_type {};

template <class Map>
struct IsMap<Map, abel::void_t<typename Map::mapped_type>> : std::true_type {};

}  // namespace generator_internal

std::mt19937_64* GetSharedRng();

enum Enum {
  kEnumEmpty,
  kEnumDeleted,
};

enum class EnumClass : uint64_t {
  kEmpty,
  kDeleted,
};

inline std::ostream& operator<<(std::ostream& o, const EnumClass& ec) {
  return o << static_cast<uint64_t>(ec);
}

template <class T, class E = void>
struct Generator;

template <class T>
struct Generator<T, typename std::enable_if<std::is_integral<T>::value>::type> {
  T operator()() const {
    std::uniform_int_distribution<T> dist;
    return dist(*GetSharedRng());
  }
};

template <>
struct Generator<Enum> {
  Enum operator()() const {
    std::uniform_int_distribution<typename std::underlying_type<Enum>::type>
        dist;
    while (true) {
      auto variate = dist(*GetSharedRng());
      if (variate != kEnumEmpty && variate != kEnumDeleted)
        return static_cast<Enum>(variate);
    }
  }
};

template <>
struct Generator<EnumClass> {
  EnumClass operator()() const {
    std::uniform_int_distribution<
        typename std::underlying_type<EnumClass>::type>
        dist;
    while (true) {
      EnumClass variate = static_cast<EnumClass>(dist(*GetSharedRng()));
      if (variate != EnumClass::kEmpty && variate != EnumClass::kDeleted)
        return static_cast<EnumClass>(variate);
    }
  }
};

template <>
struct Generator<std::string> {
  std::string operator()() const;
};

template <>
struct Generator<abel::string_view> {
  abel::string_view operator()() const;
};

template <>
struct Generator<NonStandardLayout> {
  NonStandardLayout operator()() const {
    return NonStandardLayout(Generator<std::string>()());
  }
};

template <class K, class V>
struct Generator<std::pair<K, V>> {
  std::pair<K, V> operator()() const {
    return std::pair<K, V>(Generator<typename std::decay<K>::type>()(),
                           Generator<typename std::decay<V>::type>()());
  }
};

template <class... Ts>
struct Generator<std::tuple<Ts...>> {
  std::tuple<Ts...> operator()() const {
    return std::tuple<Ts...>(Generator<typename std::decay<Ts>::type>()()...);
  }
};

template <class T>
struct Generator<std::unique_ptr<T>> {
  std::unique_ptr<T> operator()() const {
    return abel::make_unique<T>(Generator<T>()());
  }
};

template <class U>
struct Generator<U, abel::void_t<decltype(std::declval<U&>().key()),
                                decltype(std::declval<U&>().value())>>
    : Generator<std::pair<
          typename std::decay<decltype(std::declval<U&>().key())>::type,
          typename std::decay<decltype(std::declval<U&>().value())>::type>> {};

template <class Container>
using GeneratedType = decltype(
    std::declval<const Generator<
        typename std::conditional<generator_internal::IsMap<Container>::value,
                                  typename Container::value_type,
                                  typename Container::key_type>::type>&>()());

}  // namespace hash_internal
}  // namespace container_internal

}  // namespace abel

#endif  // ABEL_CONTAINER_INTERNAL_HASH_GENERATOR_TESTING_H_
