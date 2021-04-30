// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com
//
// Define the default Hash and Eq functions for SwissTable containers.
//
// std::hash<T> and std::equal_to<T> are not appropriate hash and equal
// functions for SwissTable containers. There are two reasons for this.
//
// SwissTable containers are power of 2 sized containers:
//
// This means they use the lower bits of the hash value to find the slot for
// each entry. The typical hash function for integral types is the identity.
// This is a very weak hash function for SwissTable and any power of 2 sized
// hashtable implementation which will lead to excessive collisions. For
// SwissTable we use murmur3 style mixing to reduce collisions to a minimum.
//
// SwissTable containers support heterogeneous lookup:
//
// In order to make heterogeneous lookup work, hash and equal functions must be
// polymorphic. At the same time they have to satisfy the same requirements the
// C++ standard imposes on hash functions and equality operators. That is:
//
//   if hash_default_eq<T>(a, b) returns true for any a and b of type T, then
//   hash_default_hash<T>(a) must equal hash_default_hash<T>(b)
//
// For SwissTable containers this requirement is relaxed to allow a and b of
// any and possibly different types. Note that like the standard the hash and
// equal functions are still bound to T. This is important because some type U
// can be hashed by/tested for equality differently depending on T. A notable
// example is `const char*`. `const char*` is treated as a c-style string when
// the hash function is hash<std::string> but as a pointer when the hash
// function is hash<void*>.
//
#ifndef ABEL_CONTAINER_INTERNAL_HASH_FUNCTION_DEFAULTS_H_
#define ABEL_CONTAINER_INTERNAL_HASH_FUNCTION_DEFAULTS_H_

#include <stdint.h>
#include <cstddef>
#include <memory>
#include <string>
#include <type_traits>
#include "abel/strings/case_conv.h"
#include "abel/base/profile.h"
#include "abel/hash/hash.h"
#include <string_view>

namespace abel {

    namespace container_internal {

// The hash of an object of type T is computed by using abel::hash.
        template<class T, class E = void>
        struct hash_eq {
            using Hash = abel::hash<T>;
            using Eq = std::equal_to<T>;
        };

        struct string_hash {
            using is_transparent = void;

            size_t operator()(std::string_view v) const {
                return abel::hash<std::string_view>{}(v);
            }
        };

        struct case_string_hash {
            using is_transparent = void;

            size_t operator()(std::string_view v) const {
                auto r = abel::string_to_lower(v);
                return abel::hash<std::string_view>{}(r);
            }
        };

        struct case_string_equal {
            using is_transparent = void;

            bool operator()(std::string_view lhs, std::string_view rhs) const {
                auto r = abel::string_to_lower(rhs);
                auto l = abel::string_to_lower(lhs);
                return l == r;
            }
        };

// Supports heterogeneous lookup for string-like elements.
        struct string_hash_eq {
            using Hash = string_hash;

            struct Eq {
                using is_transparent = void;

                bool operator()(std::string_view lhs, std::string_view rhs) const {
                    return lhs == rhs;
                }
            };
        };

        template<>
        struct hash_eq<std::string> : string_hash_eq {
        };
        template<>
        struct hash_eq<std::string_view> : string_hash_eq {
        };

// Supports heterogeneous lookup for pointers and smart pointers.
        template<class T>
        struct hash_eq<T *> {
            struct Hash {
                using is_transparent = void;

                template<class U>
                size_t operator()(const U &ptr) const {
                    return abel::hash<const T *>{}(hash_eq::to_ptr(ptr));
                }
            };

            struct Eq {
                using is_transparent = void;

                template<class A, class B>
                bool operator()(const A &a, const B &b) const {
                    return hash_eq::to_ptr(a) == hash_eq::to_ptr(b);
                }
            };

        private:
            static const T *to_ptr(const T *ptr) { return ptr; }

            template<class U, class D>
            static const T *to_ptr(const std::unique_ptr<U, D> &ptr) {
                return ptr.get();
            }

            template<class U>
            static const T *to_ptr(const std::shared_ptr<U> &ptr) {
                return ptr.get();
            }
        };

        template<class T, class D>
        struct hash_eq<std::unique_ptr<T, D>> : hash_eq<T *> {
        };
        template<class T>
        struct hash_eq<std::shared_ptr<T>> : hash_eq<T *> {
        };

        // This header's visibility is restricted.  If you need to access the default
        // hasher please use the container's ::hasher alias instead.
        //
        // Example: typename Hash = typename abel::flat_hash_map<K, V>::hasher
        template<class T>
        using hash_default_hash = typename container_internal::hash_eq<T>::Hash;

        // This header's visibility is restricted.  If you need to access the default
        // key equal please use the container's ::key_equal alias instead.
        //
        // Example: typename Eq = typename abel::flat_hash_map<K, V, Hash>::key_equal
        template<class T>
        using hash_default_eq = typename container_internal::hash_eq<T>::Eq;

    }  // namespace container_internal

}  // namespace abel

#endif  // ABEL_CONTAINER_INTERNAL_HASH_FUNCTION_DEFAULTS_H_
