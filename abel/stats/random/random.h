//
// -----------------------------------------------------------------------------
// File: random.h
// -----------------------------------------------------------------------------
//
// This header defines the recommended Uniform Random Bit Generator (URBG)
// types for use within the abel Random library. These types are not
// suitable for security-related use-cases, but should suffice for most other
// uses of generating random values.
//
// The abel random library provides the following URBG types:
//
//   * BitGen, a good general-purpose bit generator, optimized for generating
//     random (but not cryptographically secure) values
//   * InsecureBitGen, a slightly faster, though less random, bit generator, for
//     cases where the existing BitGen is a drag on performance.

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
// abel::BitGen
// -----------------------------------------------------------------------------
//
// `abel::BitGen` is a general-purpose random bit generator for generating
// random values for use within the abel random library. Typically, you use a
// bit generator in combination with a distribution to provide random values.
//
// Example:
//
//   // Create an abel::BitGen. There is no need to seed this bit generator.
//   abel::BitGen gen;
//
//   // Generate an integer value in the closed interval [1,6]
//   int die_roll = abel::uniform_int_distribution<int>(1, 6)(gen);
//
// `abel::BitGen` is seeded by default with non-deterministic data to produce
// different sequences of random values across different instances, including
// different binary invocations. This behavior is different than the standard
// library bit generators, which use golden values as their seeds. Default
// construction intentionally provides no stability guarantees, to avoid
// accidental dependence on such a property.
//
// `abel::BitGen` may be constructed with an optional seed sequence type,
// conforming to [rand.req.seed_seq], which will be mixed with additional
// non-deterministic data.
//
// Example:
//
//  // Create an abel::BitGen using an std::seed_seq seed sequence
//  std::seed_seq seq{1,2,3};
//  abel::BitGen gen_with_seed(seq);
//
//  // Generate an integer value in the closed interval [1,6]
//  int die_roll2 = abel::uniform_int_distribution<int>(1, 6)(gen_with_seed);
//
// `abel::BitGen` meets the requirements of the Uniform Random Bit Generator
// (URBG) concept as per the C++17 standard [rand.req.urng] though differs
// slightly with [rand.req.eng]. Like its standard library equivalents (e.g.
// `std::mersenne_twister_engine`) `abel::BitGen` is not cryptographically
// secure.
//
// Constructing two `abel::BitGen`s with the same seed sequence in the same
// binary will produce the same sequence of variates within the same binary, but
// need not do so across multiple binary invocations.
//
// This type has been optimized to perform better than Mersenne Twister
// (https://en.wikipedia.org/wiki/Mersenne_Twister) and many other complex URBG
// types on modern x86, ARM, and PPC architectures.
//
// This type is thread-compatible, but not thread-safe.

// ---------------------------------------------------------------------------
// abel::BitGen member functions
// ---------------------------------------------------------------------------

// abel::BitGen::operator()()
//
// Calls the BitGen, returning a generated value.

// abel::BitGen::min()
//
// Returns the smallest possible value from this bit generator.

// abel::BitGen::max()
//
// Returns the largest possible value from this bit generator., and

// abel::BitGen::discard(num)
//
// Advances the internal state of this bit generator by `num` times, and
// discards the intermediate results.
// ---------------------------------------------------------------------------

    using BitGen = random_internal::NonsecureURBGBase<
            random_internal::randen_engine<uint64_t>>;

// -----------------------------------------------------------------------------
// abel::InsecureBitGen
// -----------------------------------------------------------------------------
//
// `abel::InsecureBitGen` is an efficient random bit generator for generating
// random values, recommended only for performance-sensitive use cases where
// `abel::BitGen` is not satisfactory when compute-bounded by bit generation
// costs.
//
// Example:
//
//   // Create an abel::InsecureBitGen
//   abel::InsecureBitGen gen;
//   for (size_t i = 0; i < 1000000; i++) {
//
//     // Generate a bunch of random values from some complex distribution
//     auto my_rnd = some_distribution(gen, 1, 1000);
//   }
//
// Like `abel::BitGen`, `abel::InsecureBitGen` is seeded by default with
// non-deterministic data to produce different sequences of random values across
// different instances, including different binary invocations. (This behavior
// is different than the standard library bit generators, which use golden
// values as their seeds.)
//
// `abel::InsecureBitGen` may be constructed with an optional seed sequence
// type, conforming to [rand.req.seed_seq], which will be mixed with additional
// non-deterministic data. (See std_seed_seq.h for more information.)
//
// `abel::InsecureBitGen` meets the requirements of the Uniform Random Bit
// Generator (URBG) concept as per the C++17 standard [rand.req.urng] though
// its implementation differs slightly with [rand.req.eng]. Like its standard
// library equivalents (e.g. `std::mersenne_twister_engine`)
// `abel::InsecureBitGen` is not cryptographically secure.
//
// Prefer `abel::BitGen` over `abel::InsecureBitGen` as the general type is
// often fast enough for the vast majority of applications.

    using InsecureBitGen =
    random_internal::NonsecureURBGBase<random_internal::pcg64_2018_engine>;

// ---------------------------------------------------------------------------
// abel::InsecureBitGen member functions
// ---------------------------------------------------------------------------

// abel::InsecureBitGen::operator()()
//
// Calls the InsecureBitGen, returning a generated value.

// abel::InsecureBitGen::min()
//
// Returns the smallest possible value from this bit generator.

// abel::InsecureBitGen::max()
//
// Returns the largest possible value from this bit generator.

// abel::InsecureBitGen::discard(num)
//
// Advances the internal state of this bit generator by `num` times, and
// discards the intermediate results.
// ---------------------------------------------------------------------------


}  // namespace abel

#endif  // ABEL_RANDOM_RANDOM_H_
