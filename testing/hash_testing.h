// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com

#ifndef TEST_TESTING_HASH_TESTING_H_
#define TEST_TESTING_HASH_TESTING_H_

#include <initializer_list>
#include <tuple>
#include <type_traits>
#include <vector>
#include <variant>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "testing/spy_hash_state.h"
#include "abel/meta/type_traits.h"
#include "abel/strings/str_cat.h"


namespace abel {

// Run the abel::hash algorithm over all the elements passed in and verify that
// their hash expansion is congruent with their `==` operator.
//
// It is used in conjunction with EXPECT_TRUE. Failures will output information
// on what requirement failed and on which objects.
//
// Users should pass a collection of types as either an initializer list or a
// container of cases.
//
//   EXPECT_TRUE(abel::VerifyTypeImplementsAbelHashCorrectly(
//       {v1, v2, ..., vN}));
//
//   std::vector<MyType> cases;
//   // Fill cases...
//   EXPECT_TRUE(abel::VerifyTypeImplementsAbelHashCorrectly(cases));
//
// Users can pass a variety of types for testing heterogeneous lookup with
// `std::make_tuple`:
//
//   EXPECT_TRUE(abel::VerifyTypeImplementsAbelHashCorrectly(
//       std::make_tuple(v1, v2, ..., vN)));
//
//
// Ideally, the values passed should provide enough coverage of the `==`
// operator and the abel_hash_value implementations.
// For dynamically sized types, the empty state should usually be included in
// the values.
//
// The function accepts an optional comparator function, in case that `==` is
// not enough for the values provided.
//
// Usage:
//
//   EXPECT_TRUE(abel::VerifyTypeImplementsAbelHashCorrectly(
//       std::make_tuple(v1, v2, ..., vN), MyCustomEq{}));
//
// It checks the following requirements:
//   1. The expansion for a value is deterministic.
//   2. For any two objects `a` and `b` in the sequence, if `a == b` evaluates
//      to true, then their hash expansion must be equal.
//   3. If `a == b` evaluates to false their hash expansion must be unequal.
//   4. If `a == b` evaluates to false neither hash expansion can be a
//      suffix of the other.
//   5. abel_hash_value overloads should not be called by the user. They are only
//      meant to be called by the framework. Users should call H::combine() and
//      H::combine_contiguous().
//   6. No moved-from instance of the hash state is used in the implementation
//      of abel_hash_value.
//
// The values do not have to have the same type. This can be useful for
// equivalent types that support heterogeneous lookup.
//
// A possible reason for breaking (2) is combining state in the hash expansion
// that was not used in `==`.
// For example:
//
// struct Bad2 {
//   int a, b;
//   template <typename H>
//   friend H abel_hash_value(H state, Bad2 x) {
//     // Uses a and b.
//     return H::combine(std::move(state), x.a, x.b);
//   }
//   friend bool operator==(Bad2 x, Bad2 y) {
//     // Only uses a.
//     return x.a == y.a;
//   }
// };
//
// As for (3), breaking this usually means that there is state being passed to
// the `==` operator that is not used in the hash expansion.
// For example:
//
// struct Bad3 {
//   int a, b;
//   template <typename H>
//   friend H abel_hash_value(H state, Bad3 x) {
//     // Only uses a.
//     return H::combine(std::move(state), x.a);
//   }
//   friend bool operator==(Bad3 x, Bad3 y) {
//     // Uses a and b.
//     return x.a == y.a && x.b == y.b;
//   }
// };
//
// Finally, a common way to break 4 is by combining dynamic ranges without
// combining the size of the range.
// For example:
//
// struct Bad4 {
//   int *p, size;
//   template <typename H>
//   friend H abel_hash_value(H state, Bad4 x) {
//     return H::combine_contiguous(std::move(state), x.p, x.p + x.size);
//   }
//   friend bool operator==(Bad4 x, Bad4 y) {
//    // Compare two ranges for equality. C++14 code can instead use std::equal.
//     return abel::equal(x.p, x.p + x.size, y.p, y.p + y.size);
//   }
// };
//
// An easy solution to this is to combine the size after combining the range,
// like so:
// template <typename H>
// friend H abel_hash_value(H state, Bad4 x) {
//   return H::combine(
//       H::combine_contiguous(std::move(state), x.p, x.p + x.size), x.size);
// }
//
template<int &... ExplicitBarrier, typename Container>
ABEL_MUST_USE_RESULT testing::AssertionResult
VerifyTypeImplementsAbelHashCorrectly(const Container &values);

template<int &... ExplicitBarrier, typename Container, typename Eq>
ABEL_MUST_USE_RESULT testing::AssertionResult
VerifyTypeImplementsAbelHashCorrectly(const Container &values, Eq equals);

template<int &..., typename T>
ABEL_MUST_USE_RESULT testing::AssertionResult
VerifyTypeImplementsAbelHashCorrectly(std::initializer_list<T> values);

template<int &..., typename T, typename Eq>
ABEL_MUST_USE_RESULT testing::AssertionResult
VerifyTypeImplementsAbelHashCorrectly(std::initializer_list<T> values,
                                      Eq equals);

namespace hash_internal {

struct PrintVisitor {
    size_t index;

    template<typename T>
    std::string operator()(const T *value) const {
        return abel::string_cat("#", index, "(", testing::PrintToString(*value), ")");
    }
};

template<typename Eq>
struct EqVisitor {
    Eq eq;

    template<typename T, typename U>
    bool operator()(const T *t, const U *u) const {
        return eq(*t, *u);
    }
};

struct ExpandVisitor {
    template<typename T>
    spy_hash_state operator()(const T *value) const {
        return spy_hash_state::combine(spy_hash_state(), *value);
    }
};

template<typename Container, typename Eq>
ABEL_MUST_USE_RESULT testing::AssertionResult
VerifyTypeImplementsAbelHashCorrectly(const Container &values, Eq equals) {
    using V = typename Container::value_type;

    struct Info {
        const V &value;
        size_t index;

        std::string ToString() const {
            return std::visit(PrintVisitor{index}, value);
        }

        spy_hash_state expand() const { return std::visit(ExpandVisitor{}, value); }
    };

    using EqClass = std::vector<Info>;
    std::vector<EqClass> classes;

    // Gather the values in equivalence classes.
    size_t i = 0;
    for (const auto &value : values) {
        EqClass *c = nullptr;
        for (auto &eqclass : classes) {
            if (std::visit(EqVisitor<Eq>{equals}, value, eqclass[0].value)) {
                c = &eqclass;
                break;
            }
        }
        if (c == nullptr) {
            classes.emplace_back();
            c = &classes.back();
        }
        c->push_back({value, i});
        ++i;

        // Verify potential errors captured by spy_hash_state.
        if (auto error = c->back().expand().error()) {
            return testing::AssertionFailure() << *error;
        }
    }

    if (classes.size() < 2) {
        return testing::AssertionFailure()
                << "At least two equivalence classes are expected.";
    }

    // We assume that equality is correctly implemented.
    // Now we verify that abel_hash_value is also correctly implemented.

    for (const auto &c : classes) {
        // All elements of the equivalence class must have the same hash
        // expansion.
        const spy_hash_state expected = c[0].expand();
        for (const Info &v : c) {
            if (v.expand() != v.expand()) {
                return testing::AssertionFailure()
                        << "Hash expansion for " << v.ToString()
                        << " is non-deterministic.";
            }
            if (v.expand() != expected) {
                return testing::AssertionFailure()
                        << "Values " << c[0].ToString() << " and " << v.ToString()
                        << " evaluate as equal but have an unequal hash expansion.";
            }
        }

        // Elements from other classes must have different hash expansion.
        for (const auto &c2 : classes) {
            if (&c == &c2)
                continue;
            const spy_hash_state c2_hash = c2[0].expand();
            switch (spy_hash_state::Compare(expected, c2_hash)) {
                case spy_hash_state::CompareResult::kEqual:
                    return testing::AssertionFailure()
                            << "Values " << c[0].ToString() << " and " << c2[0].ToString()
                            << " evaluate as unequal but have an equal hash expansion.";
                case spy_hash_state::CompareResult::kBSuffixA:
                    return testing::AssertionFailure()
                            << "Hash expansion of " << c2[0].ToString()
                            << " is a suffix of the hash expansion of " << c[0].ToString()
                            << ".";
                case spy_hash_state::CompareResult::kASuffixB:
                    return testing::AssertionFailure()
                            << "Hash expansion of " << c[0].ToString()
                            << " is a suffix of the hash expansion of " << c2[0].ToString()
                            << ".";
                case spy_hash_state::CompareResult::kUnequal:
                    break;
            }
        }
    }
    return testing::AssertionSuccess();
}

template<typename... T>
struct TypeSet {
    template<typename U, bool = disjunction<std::is_same<T, U>...>::value>
    struct Insert {
        using type = TypeSet<U, T...>;
    };
    template<typename U>
    struct Insert<U, true> {
        using type = TypeSet;
    };

    template<template<typename...> class C>
    using apply = C<T...>;
};

template<typename... T>
struct MakeTypeSet : TypeSet<> {
};
template<typename T, typename... Ts>
struct MakeTypeSet<T, Ts...> : MakeTypeSet<Ts...>::template Insert<T>::type {
};

template<typename... T>
using VariantForTypes = typename MakeTypeSet<
        const typename std::decay<T>::type *...>::template apply<std::variant>;

template<typename Container>
struct ContainerAsVector {
    using V = std::variant<const typename Container::value_type *>;
    using Out = std::vector<V>;

    static Out Do(const Container &values) {
        Out out;
        for (const auto &v : values)
            out.push_back(&v);
        return out;
    }
};

template<typename... T>
struct ContainerAsVector<std::tuple<T...>> {
    using V = VariantForTypes<T...>;
    using Out = std::vector<V>;

    template<size_t... I>
    static Out DoImpl(const std::tuple<T...> &tuple, abel::index_sequence<I...>) {
        return Out{&std::get<I>(tuple)...};
    }

    static Out Do(const std::tuple<T...> &values) {
        return DoImpl(values, abel::index_sequence_for<T...>());
    }
};

template<>
struct ContainerAsVector<std::tuple<>> {
    static std::vector<VariantForTypes<int>> Do(std::tuple<>) { return {}; }
};

struct DefaultEquals {
    template<typename T, typename U>
    bool operator()(const T &t, const U &u) const {
        return t == u;
    }
};

}  // namespace hash_internal

template<int &..., typename Container>
ABEL_MUST_USE_RESULT testing::AssertionResult
VerifyTypeImplementsAbelHashCorrectly(const Container &values) {
    return hash_internal::VerifyTypeImplementsAbelHashCorrectly(
            hash_internal::ContainerAsVector<Container>::Do(values),
            hash_internal::DefaultEquals{});
}

template<int &..., typename Container, typename Eq>
ABEL_MUST_USE_RESULT testing::AssertionResult
VerifyTypeImplementsAbelHashCorrectly(const Container &values, Eq equals) {
    return hash_internal::VerifyTypeImplementsAbelHashCorrectly(
            hash_internal::ContainerAsVector<Container>::Do(values), equals);
}

template<int &..., typename T>
ABEL_MUST_USE_RESULT testing::AssertionResult
VerifyTypeImplementsAbelHashCorrectly(std::initializer_list<T> values) {
    return hash_internal::VerifyTypeImplementsAbelHashCorrectly(
            hash_internal::ContainerAsVector<std::initializer_list<T>>::Do(values),
            hash_internal::DefaultEquals{});
}

template<int &..., typename T, typename Eq>
ABEL_MUST_USE_RESULT testing::AssertionResult
VerifyTypeImplementsAbelHashCorrectly(std::initializer_list<T> values,
                                      Eq equals) {
    return hash_internal::VerifyTypeImplementsAbelHashCorrectly(
            hash_internal::ContainerAsVector<std::initializer_list<T>>::Do(values),
            equals);
}

}  // namespace abel

#endif  // TEST_TESTING_HASH_TESTING_H_
