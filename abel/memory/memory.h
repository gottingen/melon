// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com
//
// -----------------------------------------------------------------------------
// File: memory.h
// -----------------------------------------------------------------------------
//
// This header file contains utility functions for managing the creation and
// conversion of smart pointers. This file is an extension to the C++
// standard <memory> library header file.

#ifndef ABEL_MEMORY_MEMORY_H_
#define ABEL_MEMORY_MEMORY_H_

#include <cstddef>
#include <limits>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>

#include "abel/base/profile.h"
#include "abel/meta/type_traits.h"

namespace abel {


// -----------------------------------------------------------------------------
// Function Template: wrap_unique()
// -----------------------------------------------------------------------------
//
// Adopts ownership from a raw pointer and transfers it to the returned
// `std::unique_ptr`, whose type is deduced. Because of this deduction, *do not*
// specify the template type `T` when calling `wrap_unique`.
//
// Example:
//   X* NewX(int, int);
//   auto x = wrap_unique(NewX(1, 2));  // 'x' is std::unique_ptr<X>.
//
// Do not call wrap_unique with an explicit type, as in
// `wrap_unique<X>(NewX(1, 2))`.  The purpose of wrap_unique is to automatically
// deduce the pointer type. If you wish to make the type explicit, just use
// `std::unique_ptr` directly.
//
//   auto x = std::unique_ptr<X>(NewX(1, 2));
//                  - or -
//   std::unique_ptr<X> x(NewX(1, 2));
//
// While `abel::wrap_unique` is useful for capturing the output of a raw
// pointer factory, prefer 'abel::make_unique<T>(args...)' over
// 'abel::wrap_unique(new T(args...))'.
//
//   auto x = wrap_unique(new X(1, 2));  // works, but nonideal.
//   auto x = make_unique<X>(1, 2);     // safer, standard, avoids raw 'new'.
//
// Note that `abel::wrap_unique(p)` is valid only if `delete p` is a valid
// expression. In particular, `abel::wrap_unique()` cannot wrap pointers to
// arrays, functions or void, and it must not be used to capture pointers
// obtained from array-new expressions (even though that would compile!).
template<typename T>
std::unique_ptr<T> wrap_unique(T *ptr) {
    static_assert(!std::is_array<T>::value, "array types are unsupported");
    static_assert(std::is_object<T>::value, "non-object types are unsupported");
    return std::unique_ptr<T>(ptr);
}

namespace memory_internal {

// Traits to select proper overload and return type for `abel::make_unique<>`.
template<typename T>
struct make_unique_result {
    using scalar = std::unique_ptr<T>;
};
template<typename T>
struct make_unique_result<T[]> {
    using array = std::unique_ptr<T[]>;
};
template<typename T, size_t N>
struct make_unique_result<T[N]> {
    using invalid = void;
};

}  // namespace memory_internal

// gcc 4.8 has __cplusplus at 201301 but the libstdc++ shipped with it doesn't
// define make_unique.  Other supported compilers either just define __cplusplus
// as 201103 but have make_unique (msvc), or have make_unique whenever
// __cplusplus > 201103 (clang).
#if (__cplusplus > 201103L || defined(_MSC_VER)) && \
    !(defined(__GLIBCXX__) && !defined(__cpp_lib_make_unique))
using std::make_unique;
#else
// -----------------------------------------------------------------------------
// Function Template: make_unique<T>()
// -----------------------------------------------------------------------------
//
// Creates a `std::unique_ptr<>`, while avoiding issues creating temporaries
// during the construction process. `abel::make_unique<>` also avoids redundant
// type declarations, by avoiding the need to explicitly use the `new` operator.
//
// This implementation of `abel::make_unique<>` is designed for C++11 code and
// will be replaced in C++14 by the equivalent `std::make_unique<>` abstraction.
// `abel::make_unique<>` is designed to be 100% compatible with
// `std::make_unique<>` so that the eventual migration will involve a simple
// rename operation.
//
// For more background on why `std::unique_ptr<T>(new T(a,b))` is problematic,
// see Herb Sutter's explanation on
// (Exception-Safe Function Calls)[https://herbsutter.com/gotw/_102/].
// (In general, reviewers should treat `new T(a,b)` with scrutiny.)
//
// Example usage:
//
//    auto p = make_unique<X>(args...);  // 'p'  is a std::unique_ptr<X>
//    auto pa = make_unique<X[]>(5);     // 'pa' is a std::unique_ptr<X[]>
//
// Three overloads of `abel::make_unique` are required:
//
//   - For non-array T:
//
//       Allocates a T with `new T(std::forward<Args> args...)`,
//       forwarding all `args` to T's constructor.
//       Returns a `std::unique_ptr<T>` owning that object.
//
//   - For an array of unknown bounds T[]:
//
//       `abel::make_unique<>` will allocate an array T of type U[] with
//       `new U[n]()` and return a `std::unique_ptr<U[]>` owning that array.
//
//       Note that 'U[n]()' is different from 'U[n]', and elements will be
//       value-initialized. Note as well that `std::unique_ptr` will perform its
//       own destruction of the array elements upon leaving scope, even though
//       the array [] does not have a default destructor.
//
//       NOTE: an array of unknown bounds T[] may still be (and often will be)
//       initialized to have a size, and will still use this overload. E.g:
//
//         auto my_array = abel::make_unique<int[]>(10);
//
//   - For an array of known bounds T[N]:
//
//       `abel::make_unique<>` is deleted (like with `std::make_unique<>`) as
//       this overload is not useful.
//
//       NOTE: an array of known bounds T[N] is not considered a useful
//       construction, and may cause undefined behavior in templates. E.g:
//
//         auto my_array = abel::make_unique<int[10]>();
//
//       In those cases, of course, you can still use the overload above and
//       simply initialize it to its desired size:
//
//         auto my_array = abel::make_unique<int[]>(10);

// `abel::make_unique` overload for non-array types.
template<typename T, typename... Args>
typename memory_internal::make_unique_result<T>::scalar make_unique(
        Args &&... args) {
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

// `abel::make_unique` overload for an array T[] of unknown bounds.
// The array allocation needs to use the `new T[size]` form and cannot take
// element constructor arguments. The `std::unique_ptr` will manage destructing
// these array elements.
template<typename T>
typename memory_internal::make_unique_result<T>::array make_unique(size_t n) {
    return std::unique_ptr<T>(new typename abel::remove_extent_t<T>[n]());
}

// `abel::make_unique` overload for an array T[N] of known bounds.
// This construction will be rejected.
template<typename T, typename... Args>
typename memory_internal::make_unique_result<T>::invalid make_unique(
        Args &&... /* args */) = delete;

#endif

// -----------------------------------------------------------------------------
// Function Template: RawPtr()
// -----------------------------------------------------------------------------
//
// Extracts the raw pointer from a pointer-like value `ptr`. `abel::RawPtr` is
// useful within templates that need to handle a complement of raw pointers,
// `std::nullptr_t`, and smart pointers.
template<typename T>
auto RawPtr(T &&ptr) -> decltype(std::addressof(*ptr)) {
    // ptr is a forwarding reference to support Ts with non-const operators.
    return (ptr != nullptr) ? std::addressof(*ptr) : nullptr;
}

ABEL_FORCE_INLINE std::nullptr_t RawPtr(std::nullptr_t) { return nullptr; }

// -----------------------------------------------------------------------------
// Function Template: share_unique_ptr()
// -----------------------------------------------------------------------------
//
// Adopts a `std::unique_ptr` rvalue and returns a `std::shared_ptr` of deduced
// type. Ownership (if any) of the held value is transferred to the returned
// shared pointer.
//
// Example:
//
//     auto up = abel::make_unique<int>(10);
//     auto sp = abel::share_unique_ptr(std::move(up));  // shared_ptr<int>
//     CHECK_EQ(*sp, 10);
//     CHECK(up == nullptr);
//
// Note that this conversion is correct even when T is an array type, and more
// generally it works for *any* deleter of the `unique_ptr` (single-object
// deleter, array deleter, or any custom deleter), since the deleter is adopted
// by the shared pointer as well. The deleter is copied (unless it is a
// reference).
//
// Implements the resolution of [LWG 2415](http://wg21.link/lwg2415), by which a
// null shared pointer does not attempt to call the deleter.
template<typename T, typename D>
std::shared_ptr<T> share_unique_ptr(std::unique_ptr<T, D> &&ptr) {
    return ptr ? std::shared_ptr<T>(std::move(ptr)) : std::shared_ptr<T>();
}

// -----------------------------------------------------------------------------
// Function Template: weaken_ptr()
// -----------------------------------------------------------------------------
//
// Creates a weak pointer associated with a given shared pointer. The returned
// value is a `std::weak_ptr` of deduced type.
//
// Example:
//
//    auto sp = std::make_shared<int>(10);
//    auto wp = abel::weaken_ptr(sp);
//    CHECK_EQ(sp.get(), wp.lock().get());
//    sp.reset();
//    CHECK(wp.lock() == nullptr);
//
template<typename T>
std::weak_ptr<T> weaken_ptr(const std::shared_ptr<T> &ptr) {
    return std::weak_ptr<T>(ptr);
}

namespace memory_internal {

// ExtractOr<E, O, D>::type evaluates to E<O> if possible. Otherwise, D.
template<template<typename> class Extract, typename Obj, typename Default,
        typename>
struct ExtractOr {
    using type = Default;
};

template<template<typename> class Extract, typename Obj, typename Default>
struct ExtractOr<Extract, Obj, Default, void_t < Extract<Obj>>> {
using type = Extract<Obj>;
};

template<template<typename> class Extract, typename Obj, typename Default>
using ExtractOrT = typename ExtractOr<Extract, Obj, Default, void>::type;

// Extractors for the features of allocators.
template<typename T>
using get_pointer = typename T::pointer;

template<typename T>
using get_const_pointer = typename T::const_pointer;

template<typename T>
using get_void_pointer = typename T::void_pointer;

template<typename T>
using get_const_void_pointer = typename T::const_void_pointer;

template<typename T>
using get_difference_type = typename T::difference_type;

template<typename T>
using get_size_type = typename T::size_type;

template<typename T>
using get_propagate_on_container_copy_assignment =
typename T::propagate_on_container_copy_assignment;

template<typename T>
using get_propagate_on_container_move_assignment =
typename T::propagate_on_container_move_assignment;

template<typename T>
using get_propagate_on_container_swap = typename T::propagate_on_container_swap;

template<typename T>
using getIs_always_equal = typename T::is_always_equal;

template<typename T>
struct get_first_arg;

template<template<typename...> class Class, typename T, typename... Args>
struct get_first_arg<Class<T, Args...>> {
    using type = T;
};

template<typename Ptr, typename = void>
struct element_type {
    using type = typename get_first_arg<Ptr>::type;
};

template<typename T>
struct element_type<T, void_t < typename T::element_type>> {
using type = typename T::element_type;
};

template<typename T, typename U>
struct rebind_first_arg;

template<template<typename...> class Class, typename T, typename... Args,
        typename U>
struct rebind_first_arg<Class<T, Args...>, U> {
    using type = Class<U, Args...>;
};

template<typename T, typename U, typename = void>
struct rebind_ptr {
    using type = typename rebind_first_arg<T, U>::type;
};

template<typename T, typename U>
struct rebind_ptr<T, U, void_t < typename T::template rebind<U>>> {
using type = typename T::template rebind<U>;
};

template<typename T, typename U>
constexpr bool has_rebind_alloc(...) {
    return false;
}

template<typename T, typename U>
constexpr bool has_rebind_alloc(typename T::template rebind<U>::other *) {
    return true;
}

template<typename T, typename U, bool = has_rebind_alloc<T, U>(nullptr)>
struct rebind_alloc {
    using type = typename rebind_first_arg<T, U>::type;
};

template<typename T, typename U>
struct rebind_alloc<T, U, true> {
    using type = typename T::template rebind<U>::other;
};

}  // namespace memory_internal

// -----------------------------------------------------------------------------
// Class Template: pointer_traits
// -----------------------------------------------------------------------------
//
// An implementation of C++11's std::pointer_traits.
//
// Provided for portability on toolchains that have a working C++11 compiler,
// but the standard library is lacking in C++11 support. For example, some
// version of the Android NDK.
//

template<typename Ptr>
struct pointer_traits {
    using pointer = Ptr;

    // element_type:
    // Ptr::element_type if present. Otherwise T if Ptr is a template
    // instantiation Template<T, Args...>
    using element_type = typename memory_internal::element_type<Ptr>::type;

    // difference_type:
    // Ptr::difference_type if present, otherwise std::ptrdiff_t
    using difference_type =
    memory_internal::ExtractOrT<memory_internal::get_difference_type, Ptr,
            std::ptrdiff_t>;

    // rebind:
    // Ptr::rebind<U> if exists, otherwise Template<U, Args...> if Ptr is a
    // template instantiation Template<T, Args...>
    template<typename U>
    using rebind = typename memory_internal::rebind_ptr<Ptr, U>::type;

    // pointer_to:
    // Calls Ptr::pointer_to(r)
    static pointer pointer_to(element_type &r) {  // NOLINT(runtime/references)
        return Ptr::pointer_to(r);
    }
};

// Specialization for T*.
template<typename T>
struct pointer_traits<T *> {
    using pointer = T *;
    using element_type = T;
    using difference_type = std::ptrdiff_t;

    template<typename U>
    using rebind = U *;

    // pointer_to:
    // Calls std::addressof(r)
    static pointer pointer_to(
            element_type &r) noexcept {  // NOLINT(runtime/references)
        return std::addressof(r);
    }
};

// -----------------------------------------------------------------------------
// Class Template: allocator_traits
// -----------------------------------------------------------------------------
//
// A C++11 compatible implementation of C++17's std::allocator_traits.
//
template<typename Alloc>
struct allocator_traits {
    using allocator_type = Alloc;

    // value_type:
    // Alloc::value_type
    using value_type = typename Alloc::value_type;

    // pointer:
    // Alloc::pointer if present, otherwise value_type*
    using pointer = memory_internal::ExtractOrT<memory_internal::get_pointer,
            Alloc, value_type *>;

    // const_pointer:
    // Alloc::const_pointer if present, otherwise
    // abel::pointer_traits<pointer>::rebind<const value_type>
    using const_pointer =
    memory_internal::ExtractOrT<memory_internal::get_const_pointer, Alloc,
            typename abel::pointer_traits<pointer>::
            template rebind<const value_type>>;

    // void_pointer:
    // Alloc::void_pointer if present, otherwise
    // abel::pointer_traits<pointer>::rebind<void>
    using void_pointer = memory_internal::ExtractOrT<
            memory_internal::get_void_pointer, Alloc,
            typename abel::pointer_traits<pointer>::template rebind<void>>;

    // const_void_pointer:
    // Alloc::const_void_pointer if present, otherwise
    // abel::pointer_traits<pointer>::rebind<const void>
    using const_void_pointer = memory_internal::ExtractOrT<
            memory_internal::get_const_void_pointer, Alloc,
            typename abel::pointer_traits<pointer>::template rebind<const void>>;

    // difference_type:
    // Alloc::difference_type if present, otherwise
    // abel::pointer_traits<pointer>::difference_type
    using difference_type = memory_internal::ExtractOrT<
            memory_internal::get_difference_type, Alloc,
            typename abel::pointer_traits<pointer>::difference_type>;

    // size_type:
    // Alloc::size_type if present, otherwise
    // std::make_unsigned<difference_type>::type
    using size_type = memory_internal::ExtractOrT<
            memory_internal::get_size_type, Alloc,
            typename std::make_unsigned<difference_type>::type>;

    // propagate_on_container_copy_assignment:
    // Alloc::propagate_on_container_copy_assignment if present, otherwise
    // std::false_type
    using propagate_on_container_copy_assignment = memory_internal::ExtractOrT<
            memory_internal::get_propagate_on_container_copy_assignment, Alloc,
            std::false_type>;

    // propagate_on_container_move_assignment:
    // Alloc::propagate_on_container_move_assignment if present, otherwise
    // std::false_type
    using propagate_on_container_move_assignment = memory_internal::ExtractOrT<
            memory_internal::get_propagate_on_container_move_assignment, Alloc,
            std::false_type>;

    // propagate_on_container_swap:
    // Alloc::propagate_on_container_swap if present, otherwise std::false_type
    using propagate_on_container_swap =
    memory_internal::ExtractOrT<memory_internal::get_propagate_on_container_swap,
            Alloc, std::false_type>;

    // is_always_equal:
    // Alloc::is_always_equal if present, otherwise std::is_empty<Alloc>::type
    using is_always_equal =
    memory_internal::ExtractOrT<memory_internal::getIs_always_equal, Alloc,
            typename std::is_empty<Alloc>::type>;

    // rebind_alloc:
    // Alloc::rebind<T>::other if present, otherwise Alloc<T, Args> if this Alloc
    // is Alloc<U, Args>
    template<typename T>
    using rebind_alloc = typename memory_internal::rebind_alloc<Alloc, T>::type;

    // rebind_traits:
    // abel::allocator_traits<rebind_alloc<T>>
    template<typename T>
    using rebind_traits = abel::allocator_traits<rebind_alloc<T>>;

    // allocate(Alloc& a, size_type n):
    // Calls a.allocate(n)
    static pointer allocate(Alloc &a,  // NOLINT(runtime/references)
                            size_type n) {
        return a.allocate(n);
    }

    // allocate(Alloc& a, size_type n, const_void_pointer hint):
    // Calls a.allocate(n, hint) if possible.
    // If not possible, calls a.allocate(n)
    static pointer allocate(Alloc &a, size_type n,  // NOLINT(runtime/references)
                            const_void_pointer hint) {
        return allocate_impl(0, a, n, hint);
    }

    // deallocate(Alloc& a, pointer p, size_type n):
    // Calls a.deallocate(p, n)
    static void deallocate(Alloc &a, pointer p,  // NOLINT(runtime/references)
                           size_type n) {
        a.deallocate(p, n);
    }

    // construct(Alloc& a, T* p, Args&&... args):
    // Calls a.construct(p, std::forward<Args>(args)...) if possible.
    // If not possible, calls
    //   ::new (static_cast<void*>(p)) T(std::forward<Args>(args)...)
    template<typename T, typename... Args>
    static void construct(Alloc &a, T *p,  // NOLINT(runtime/references)
                          Args &&... args) {
        construct_impl(0, a, p, std::forward<Args>(args)...);
    }

    // destroy(Alloc& a, T* p):
    // Calls a.destroy(p) if possible. If not possible, calls p->~T().
    template<typename T>
    static void destroy(Alloc &a, T *p) {  // NOLINT(runtime/references)
        destroy_impl(0, a, p);
    }

    // max_size(const Alloc& a):
    // Returns a.max_size() if possible. If not possible, returns
    //   std::numeric_limits<size_type>::max() / sizeof(value_type)
    static size_type max_size(const Alloc &a) { return max_size_impl(0, a); }

    // select_on_container_copy_construction(const Alloc& a):
    // Returns a.select_on_container_copy_construction() if possible.
    // If not possible, returns a.
    static Alloc select_on_container_copy_construction(const Alloc &a) {
        return select_on_container_copy_construction_impl(0, a);
    }

  private:
    template<typename A>
    static auto allocate_impl(int, A &a,  // NOLINT(runtime/references)
                              size_type n, const_void_pointer hint)
    -> decltype(a.allocate(n, hint)) {
        return a.allocate(n, hint);
    }

    static pointer allocate_impl(char, Alloc &a,  // NOLINT(runtime/references)
                                 size_type n, const_void_pointer) {
        return a.allocate(n);
    }

    template<typename A, typename... Args>
    static auto construct_impl(int, A &a,  // NOLINT(runtime/references)
                               Args &&... args)
    -> decltype(a.construct(std::forward<Args>(args)...)) {
        a.construct(std::forward<Args>(args)...);
    }

    template<typename T, typename... Args>
    static void construct_impl(char, Alloc &, T *p, Args &&... args) {
        ::new(static_cast<void *>(p)) T(std::forward<Args>(args)...);
    }

    template<typename A, typename T>
    static auto destroy_impl(int, A &a,  // NOLINT(runtime/references)
                             T *p) -> decltype(a.destroy(p)) {
        a.destroy(p);
    }

    template<typename T>
    static void destroy_impl(char, Alloc &, T *p) {
        p->~T();
    }

    template<typename A>
    static auto max_size_impl(int, const A &a) -> decltype(a.max_size()) {
        return a.max_size();
    }

    static size_type max_size_impl(char, const Alloc &) {
        return (std::numeric_limits<size_type>::max)() / sizeof(value_type);
    }

    template<typename A>
    static auto select_on_container_copy_construction_impl(int, const A &a)
    -> decltype(a.select_on_container_copy_construction()) {
        return a.select_on_container_copy_construction();
    }

    static Alloc select_on_container_copy_construction_impl(char,
                                                            const Alloc &a) {
        return a;
    }
};

namespace memory_internal {

// This template alias transforms Alloc::is_nothrow into a metafunction with
// Alloc as a parameter so it can be used with ExtractOrT<>.
template<typename Alloc>
using GetIsNothrow = typename Alloc::is_nothrow;

}  // namespace memory_internal

// ABEL_ALLOCATOR_NOTHROW is a build time configuration macro for user to
// specify whether the default allocation function can throw or never throws.
// If the allocation function never throws, user should define it to a non-zero
// value (e.g. via `-DABEL_ALLOCATOR_NOTHROW`).
// If the allocation function can throw, user should leave it undefined or
// define it to zero.
//
// allocator_is_nothrow<Alloc> is a traits class that derives from
// Alloc::is_nothrow if present, otherwise std::false_type. It's specialized
// for Alloc = std::allocator<T> for any type T according to the state of
// ABEL_ALLOCATOR_NOTHROW.
//
// default_allocator_is_nothrow is a class that derives from std::true_type
// when the default allocator (global operator new) never throws, and
// std::false_type when it can throw. It is a convenience shorthand for writing
// allocator_is_nothrow<std::allocator<T>> (T can be any type).
// NOTE: allocator_is_nothrow<std::allocator<T>> is guaranteed to derive from
// the same type for all T, because users should specialize neither
// allocator_is_nothrow nor std::allocator.
template<typename Alloc>
struct allocator_is_nothrow
        : memory_internal::ExtractOrT<memory_internal::GetIsNothrow, Alloc,
                std::false_type> {
};

#if defined(ABEL_ALLOCATOR_NOTHROW) && ABEL_ALLOCATOR_NOTHROW
template <typename T>
struct allocator_is_nothrow<std::allocator<T>> : std::true_type {};
struct default_allocator_is_nothrow : std::true_type {};
#else
struct default_allocator_is_nothrow : std::false_type {
};
#endif

namespace memory_internal {
template<typename Allocator, typename Iterator, typename... Args>
void ConstructRange(Allocator &alloc, Iterator first, Iterator last,
                    const Args &... args) {
    for (Iterator cur = first; cur != last; ++cur) {
        ABEL_TRY {
            std::allocator_traits<Allocator>::construct(alloc, std::addressof(*cur),
                                                        args...);
        } ABEL_CATCH (...) {
            while (cur != first) {
                --cur;
                std::allocator_traits<Allocator>::destroy(alloc, std::addressof(*cur));
            }
            ABEL_RETHROW;
        }
    }
}

template<typename Allocator, typename Iterator, typename InputIterator>
void CopyRange(Allocator &alloc, Iterator destination, InputIterator first,
               InputIterator last) {
    for (Iterator cur = destination; first != last;
         static_cast<void>(++cur), static_cast<void>(++first)) {
        ABEL_TRY {
            std::allocator_traits<Allocator>::construct(alloc, std::addressof(*cur),
                                                        *first);
        } ABEL_CATCH (...) {
            while (cur != destination) {
                --cur;
                std::allocator_traits<Allocator>::destroy(alloc, std::addressof(*cur));
            }
            ABEL_RETHROW;
        }
    }
}
}  // namespace memory_internal

}  // namespace abel

#endif  // ABEL_MEMORY_MEMORY_H_
