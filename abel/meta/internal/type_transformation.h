// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com


#ifndef ABEL_META_INTERNAL_TYPE_TRANSFORMATION_H_
#define ABEL_META_INTERNAL_TYPE_TRANSFORMATION_H_

#include <limits>
#include "abel/meta/internal/config.h"
#include "abel/meta/internal/type_common.h"

namespace abel {

// void_t()
//
// Ignores the type of any its arguments and returns `void`. In general, this
// metafunction allows you to create a general case that maps to `void` while
// allowing specializations that map to specific types.
//
// This metafunction is designed to be a drop-in replacement for the C++17
// `std::void_t` metafunction.
//
// NOTE: `abel::void_t` does not use the standard-specified implementation so
// that it can remain compatible with gcc < 5.1. This can introduce slightly
// different behavior, such as when ordering partial specializations.

template<typename... Ts>
using void_t = typename asl_internal::void_t_impl<Ts...>::type;


template<typename To>
constexpr To implicit_cast(typename abel::type_identity_t<To> to) {
    return to;
}


// unsigned_bits<N>::type returns the unsigned int type with the indicated
// number of bits.
template<size_t N>
struct unsigned_bits;

template<>
struct unsigned_bits<8> {
    using type = uint8_t;
};
template<>
struct unsigned_bits<16> {
    using type = uint16_t;
};
template<>
struct unsigned_bits<32> {
    using type = uint32_t;
};
template<>
struct unsigned_bits<64> {
    using type = uint64_t;
};

#ifdef ABEL_HAVE_INTRINSIC_INT128
template<>
struct unsigned_bits<128> {
    using type = __uint128_t;
};
#endif

template<typename IntType>
struct make_unsigned_bits {
    using type = typename unsigned_bits<std::numeric_limits<
            typename std::make_unsigned<IntType>::type>::digits>::type;
};


using std::swap;

// This declaration prevents global `swap` and `abel::swap` overloads from being
// considered unless ADL picks them up.
void swap();

template<class T>
using is_swappable_impl = decltype(swap(std::declval<T &>(), std::declval<T &>()));

// NOTE: This dance with the default template parameter is for MSVC.
template<class T,
        class is_noexcept = std::integral_constant<
                bool, noexcept(swap(std::declval<T &>(), std::declval<T &>()))>>
using is_nothrow_swappable_impl = typename std::enable_if<is_noexcept::value>::type;


// is_swappable
//
// Determines whether the standard swap idiom is a valid expression for
// arguments of type `T`.
template<class T>
struct is_swappable
        : abel::is_detected<is_swappable_impl, T> {
};

// is_nothrow_swappable
//
// Determines whether the standard swap idiom is a valid expression for
// arguments of type `T` and is noexcept.
template<class T>
struct is_nothrow_swappable
        : abel::is_detected<is_nothrow_swappable_impl, T> {
};



// abel_swap()
//
// Performs the swap idiom from a namespace where valid candidates may only be
// found in `std` or via ADL.
template<class T, abel::enable_if_t<is_swappable<T>::value, int> = 0>
void abel_swap(T &lhs, T &rhs) noexcept(is_nothrow_swappable<T>::value) {
    swap(lhs, rhs);
}

// std_swap_is_unconstrained
//
// Some standard library implementations are broken in that they do not
// constrain `std::swap`. This will effectively tell us if we are dealing with
// one of those implementations.
using std_swap_is_unconstrained = is_swappable<void()>;

}  // namespace abel

#endif  // ABEL_META_INTERNAL_TYPE_TRANSFORMATION_H_
