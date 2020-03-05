//
// -----------------------------------------------------------------------------
// File: random.h
// -----------------------------------------------------------------------------
//
// This header defines the recommended uniform Random Bit Generator (URBG)
// types for use within the abel Random library. These types are not
// suitable for security-related use-cases, but should suffice for most other
// uses of generating random values.
//
// The abel random library provides the following URBG types:
//
//   * bit_gen, a good general-purpose bit generator, optimized for generating
//     random (but not cryptographically secure) values
//   * insecure_bit_gen, a slightly faster, though less random, bit generator, for
//     cases where the existing bit_gen is a drag on performance.

#ifndef ABEL_RANDOM_RANDOM_H_
#define ABEL_RANDOM_RANDOM_H_

#include <random>

#include <abel/stats/random/distributions.h>  // IWYU pragma: export
#include <abel/stats/random/engine/nonsecure_base.h>  // IWYU pragma: export
#include <abel/stats/random/engine/pcg_engine.h>  // IWYU pragma: export
#include <abel/stats/random/engine/pool_urbg.h>
#include <abel/stats/random/engine/randen_engine.h>
#include <abel/stats/random/seed_sequences.h>  // IWYU pragma: export

namespace abel {


// -----------------------------------------------------------------------------
// abel::bit_gen
// -----------------------------------------------------------------------------
//
// `abel::bit_gen` is a general-purpose random bit generator for generating
// random values for use within the abel random library. Typically, you use a
// bit generator in combination with a distribution to provide random values.
//
// Example:
//
//   // Create an abel::bit_gen. There is no need to seed this bit generator.
//   abel::bit_gen gen;
//
//   // Generate an integer value in the closed interval [1,6]
//   int die_roll = abel::uniform_int_distribution<int>(1, 6)(gen);
//
// `abel::bit_gen` is seeded by default with non-deterministic data to produce
// different sequences of random values across different instances, including
// different binary invocations. This behavior is different than the standard
// library bit generators, which use golden values as their seeds. Default
// construction intentionally provides no stability guarantees, to avoid
// accidental dependence on such a property.
//
// `abel::bit_gen` may be constructed with an optional seed sequence type,
// conforming to [rand.req.seed_seq], which will be mixed with additional
// non-deterministic data.
//
// Example:
//
//  // Create an abel::bit_gen using an std::seed_seq seed sequence
//  std::seed_seq seq{1,2,3};
//  abel::bit_gen gen_with_seed(seq);
//
//  // Generate an integer value in the closed interval [1,6]
//  int die_roll2 = abel::uniform_int_distribution<int>(1, 6)(gen_with_seed);
//
// `abel::bit_gen` meets the requirements of the uniform Random Bit Generator
// (URBG) concept as per the C++17 standard [rand.req.urng] though differs
// slightly with [rand.req.eng]. Like its standard library equivalents (e.g.
// `std::mersenne_twister_engine`) `abel::bit_gen` is not cryptographically
// secure.
//
// Constructing two `abel::bit_gen`s with the same seed sequence in the same
// binary will produce the same sequence of variates within the same binary, but
// need not do so across multiple binary invocations.
//
// This type has been optimized to perform better than Mersenne Twister
// (https://en.wikipedia.org/wiki/Mersenne_Twister) and many other complex URBG
// types on modern x86, ARM, and PPC architectures.
//
// This type is thread-compatible, but not thread-safe.

// ---------------------------------------------------------------------------
// abel::bit_gen member functions
// ---------------------------------------------------------------------------

// abel::bit_gen::operator()()
//
// Calls the bit_gen, returning a generated value.

// abel::bit_gen::min()
//
// Returns the smallest possible value from this bit generator.

// abel::bit_gen::max()
//
// Returns the largest possible value from this bit generator., and

// abel::bit_gen::discard(num)
//
// Advances the internal state of this bit generator by `num` times, and
// discards the intermediate results.
// ---------------------------------------------------------------------------

    using bit_gen = random_internal::nonsecure_urbg_base<
            random_internal::randen_engine<uint64_t>>;

// -----------------------------------------------------------------------------
// abel::insecure_bit_gen
// -----------------------------------------------------------------------------
//
// `abel::insecure_bit_gen` is an efficient random bit generator for generating
// random values, recommended only for performance-sensitive use cases where
// `abel::bit_gen` is not satisfactory when compute-bounded by bit generation
// costs.
//
// Example:
//
//   // Create an abel::insecure_bit_gen
//   abel::insecure_bit_gen gen;
//   for (size_t i = 0; i < 1000000; i++) {
//
//     // Generate a bunch of random values from some complex distribution
//     auto my_rnd = some_distribution(gen, 1, 1000);
//   }
//
// Like `abel::bit_gen`, `abel::insecure_bit_gen` is seeded by default with
// non-deterministic data to produce different sequences of random values across
// different instances, including different binary invocations. (This behavior
// is different than the standard library bit generators, which use golden
// values as their seeds.)
//
// `abel::insecure_bit_gen` may be constructed with an optional seed sequence
// type, conforming to [rand.req.seed_seq], which will be mixed with additional
// non-deterministic data. (See std_seed_seq.h for more information.)
//
// `abel::insecure_bit_gen` meets the requirements of the uniform Random Bit
// Generator (URBG) concept as per the C++17 standard [rand.req.urng] though
// its implementation differs slightly with [rand.req.eng]. Like its standard
// library equivalents (e.g. `std::mersenne_twister_engine`)
// `abel::insecure_bit_gen` is not cryptographically secure.
//
// Prefer `abel::bit_gen` over `abel::insecure_bit_gen` as the general type is
// often fast enough for the vast majority of applications.

    using insecure_bit_gen =
    random_internal::nonsecure_urbg_base<random_internal::pcg64_2018_engine>;

// ---------------------------------------------------------------------------
// abel::insecure_bit_gen member functions
// ---------------------------------------------------------------------------

// abel::insecure_bit_gen::operator()()
//
// Calls the insecure_bit_gen, returning a generated value.

// abel::insecure_bit_gen::min()
//
// Returns the smallest possible value from this bit generator.

// abel::insecure_bit_gen::max()
//
// Returns the largest possible value from this bit generator.

// abel::insecure_bit_gen::discard(num)
//
// Advances the internal state of this bit generator by `num` times, and
// discards the intermediate results.
// ---------------------------------------------------------------------------


}  // namespace abel

#endif  // ABEL_RANDOM_RANDOM_H_
