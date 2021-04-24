// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com

// Implementation details for `abel::bind_front()`.

#ifndef ABEL_FUNCTIONAL_INTERNAL_FRONT_BINDER_H_
#define ABEL_FUNCTIONAL_INTERNAL_FRONT_BINDER_H_

#include <cstddef>
#include <type_traits>
#include <utility>

#include "abel/functional/invoke.h"
#include "abel/container/internal/compressed_tuple.h"
#include "abel/meta/type_traits.h"
#include "abel/utility/utility.h"

namespace abel {

namespace functional_internal {

// Invoke the method, expanding the tuple of bound arguments.
template<class R, class Tuple, size_t... Idx, class... Args>
R Apply(Tuple &&bound, abel::index_sequence<Idx...>, Args &&... free) {
    return base_internal::Invoke(
            abel::forward<Tuple>(bound).template get<Idx>()...,
            abel::forward<Args>(free)...);
}

template<class F, class... BoundArgs>
class FrontBinder {
    using BoundArgsT = abel::container_internal::compressed_tuple<F, BoundArgs...>;
    using Idx = abel::make_index_sequence<sizeof...(BoundArgs) + 1>;

    BoundArgsT bound_args_;

  public:
    template<class... Ts>
    constexpr explicit FrontBinder(abel::in_place_t, Ts &&... ts)
            : bound_args_(abel::forward<Ts>(ts)...) {}

    template<class... FreeArgs,
            class R = base_internal::InvokeT<F &, BoundArgs &..., FreeArgs &&...>>
    R operator()(FreeArgs &&... free_args) &{
        return functional_internal::Apply<R>(bound_args_, Idx(),
                                             abel::forward<FreeArgs>(free_args)...);
    }

    template<class... FreeArgs,
            class R = base_internal::InvokeT<const F &, const BoundArgs &...,
                    FreeArgs &&...>>
    R operator()(FreeArgs &&... free_args) const &{
        return functional_internal::Apply<R>(bound_args_, Idx(),
                                             abel::forward<FreeArgs>(free_args)...);
    }

    template<class... FreeArgs, class R = base_internal::InvokeT<
            F &&, BoundArgs &&..., FreeArgs &&...>>
    R operator()(FreeArgs &&... free_args) &&{
        // This overload is called when *this is an rvalue. If some of the bound
        // arguments are stored by value or rvalue reference, we move them.
        return functional_internal::Apply<R>(abel::move(bound_args_), Idx(),
                                             abel::forward<FreeArgs>(free_args)...);
    }

    template<class... FreeArgs,
            class R = base_internal::InvokeT<const F &&, const BoundArgs &&...,
                    FreeArgs &&...>>
    R operator()(FreeArgs &&... free_args) const &&{
        // This overload is called when *this is an rvalue. If some of the bound
        // arguments are stored by value or rvalue reference, we move them.
        return functional_internal::Apply<R>(abel::move(bound_args_), Idx(),
                                             abel::forward<FreeArgs>(free_args)...);
    }
};

template<class F, class... BoundArgs>
using bind_front_t = FrontBinder<decay_t<F>, abel::decay_t<BoundArgs>...>;

}  // namespace functional_internal

}  // namespace abel

#endif  // ABEL_FUNCTIONAL_INTERNAL_FRONT_BINDER_H_
