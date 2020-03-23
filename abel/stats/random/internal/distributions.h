//

#ifndef ABEL_RANDOM_INTERNAL_DISTRIBUTIONS_H_
#define ABEL_RANDOM_INTERNAL_DISTRIBUTIONS_H_


#include <abel/asl/type_traits.h>
#include <abel/stats/random/internal/distribution_caller.h>
#include <abel/stats/random/internal/uniform_helper.h>

namespace abel {

    namespace random_internal {

// In the absence of an explicitly provided return-type, the template
// "uniform_inferred_return_t<A, B>" is used to derive a suitable type, based on
// the data-types of the endpoint-arguments {A lo, B hi}.
//
// Given endpoints {A lo, B hi}, one of {A, B} will be chosen as the
// return-type, if one type can be implicitly converted into the other, in a
// lossless way. The template "is_widening_convertible" implements the
// compile-time logic for deciding if such a conversion is possible.
//
// If no such conversion between {A, B} exists, then the overload for
// abel::uniform() will be discarded, and the call will be ill-formed.
// Return-type for abel::uniform() when the return-type is inferred.
        template<typename A, typename B>
        using uniform_inferred_return_t =
        abel::enable_if_t<abel::disjunction<abel::is_widening_convertible<A, B>,
                is_widening_convertible<B, A>>::value,
                typename std::conditional<
                        is_widening_convertible<A, B>::value, B, A>::type>;

    }  // namespace random_internal

}  // namespace abel

#endif  // ABEL_RANDOM_INTERNAL_DISTRIBUTIONS_H_
