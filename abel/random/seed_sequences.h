//
// -----------------------------------------------------------------------------
// File: seed_sequences.h
// -----------------------------------------------------------------------------
//
// This header contains utilities for creating and working with seed sequences
// conforming to [rand.req.seedseq]. In general, direct construction of seed
// sequences is discouraged, but use-cases for construction of identical bit
// generators (using the same seed sequence) may be helpful (e.g. replaying a
// simulation whose state is derived from variates of a bit generator).

#ifndef ABEL_RANDOM_SEED_SEQUENCES_H_
#define ABEL_RANDOM_SEED_SEQUENCES_H_

#include <iterator>
#include <random>

#include <abel/random/internal/salted_seed_seq.h>
#include <abel/random/internal/seed_material.h>
#include <abel/random/seed_gen_exception.h>
#include <abel/types/span.h>

namespace abel {


// -----------------------------------------------------------------------------
// abel::SeedSeq
// -----------------------------------------------------------------------------
//
// `abel::SeedSeq` constructs a seed sequence according to [rand.req.seedseq]
// for use within bit generators. `abel::SeedSeq`, unlike `std::seed_seq`
// additionally salts the generated seeds with extra implementation-defined
// entropy. For that reason, you can use `abel::SeedSeq` in combination with
// standard library bit generators (e.g. `std::mt19937`) to introduce
// non-determinism in your seeds.
//
// Example:
//
//   abel::SeedSeq my_seed_seq({a, b, c});
//   std::mt19937 my_bitgen(my_seed_seq);
//
using SeedSeq = random_internal::SaltedSeedSeq<std::seed_seq>;

// -----------------------------------------------------------------------------
// abel::CreateSeedSeqFrom(bitgen*)
// -----------------------------------------------------------------------------
//
// Constructs a seed sequence conforming to [rand.req.seedseq] using variates
// produced by a provided bit generator.
//
// You should generally avoid direct construction of seed sequences, but
// use-cases for reuse of a seed sequence to construct identical bit generators
// may be helpful (eg. replaying a simulation whose state is derived from bit
// generator values).
//
// If bitgen == nullptr, then behavior is undefined.
//
// Example:
//
//   abel::BitGen my_bitgen;
//   auto seed_seq = abel::CreateSeedSeqFrom(&my_bitgen);
//   abel::BitGen new_engine(seed_seq); // derived from my_bitgen, but not
//                                      // correlated.
//
template <typename URBG>
SeedSeq CreateSeedSeqFrom(URBG* urbg) {
  SeedSeq::result_type
      seed_material[random_internal::kEntropyBlocksNeeded];

  if (!random_internal::ReadSeedMaterialFromURBG(
          urbg, abel::MakeSpan(seed_material))) {
    random_internal::ThrowSeedGenException();
  }
  return SeedSeq(std::begin(seed_material), std::end(seed_material));
}

// -----------------------------------------------------------------------------
// abel::MakeSeedSeq()
// -----------------------------------------------------------------------------
//
// Constructs an `abel::SeedSeq` salting the generated values using
// implementation-defined entropy. The returned sequence can be used to create
// equivalent bit generators correlated using this sequence.
//
// Example:
//
//   auto my_seed_seq = abel::MakeSeedSeq();
//   std::mt19937 rng1(my_seed_seq);
//   std::mt19937 rng2(my_seed_seq);
//   EXPECT_EQ(rng1(), rng2());
//
SeedSeq MakeSeedSeq();


}  // namespace abel

#endif  // ABEL_RANDOM_SEED_SEQUENCES_H_
