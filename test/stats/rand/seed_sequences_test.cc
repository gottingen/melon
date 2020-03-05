//

#include <abel/stats/random/seed_sequences.h>

#include <iterator>
#include <random>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <abel/stats/random/engine/nonsecure_base.h>
#include <abel/stats/random/random.h>

namespace {

    TEST(SeedSequences, Examples) {
        {
            abel::SeedSeq seed_seq({1, 2, 3});
            abel::bit_gen bitgen(seed_seq);

            EXPECT_NE(0, bitgen());
        }
        {
            abel::bit_gen engine;
            auto seed_seq = abel::CreateSeedSeqFrom(&engine);
            abel::bit_gen bitgen(seed_seq);

            EXPECT_NE(engine(), bitgen());
        }
        {
            auto seed_seq = abel::MakeSeedSeq();
            std::mt19937 random(seed_seq);

            EXPECT_NE(0, random());
        }
    }

    TEST(CreateSeedSeqFrom, CompatibleWithStdTypes) {
        using ExampleNonsecureURBG =
        abel::random_internal::nonsecure_urbg_base<std::minstd_rand0>;

        // Construct a URBG instance.
        ExampleNonsecureURBG rng;

        // Construct a Seed Sequence from its variates.
        auto seq_from_rng = abel::CreateSeedSeqFrom(&rng);

        // Ensure that another URBG can be validly constructed from the Seed Sequence.
        std::mt19937_64{seq_from_rng};
    }

    TEST(CreateSeedSeqFrom, CompatibleWithBitGenerator) {
        // Construct a URBG instance.
        abel::bit_gen rng;

        // Construct a Seed Sequence from its variates.
        auto seq_from_rng = abel::CreateSeedSeqFrom(&rng);

        // Ensure that another URBG can be validly constructed from the Seed Sequence.
        std::mt19937_64{seq_from_rng};
    }

    TEST(CreateSeedSeqFrom, CompatibleWithInsecureBitGen) {
        // Construct a URBG instance.
        abel::insecure_bit_gen rng;

        // Construct a Seed Sequence from its variates.
        auto seq_from_rng = abel::CreateSeedSeqFrom(&rng);

        // Ensure that another URBG can be validly constructed from the Seed Sequence.
        std::mt19937_64{seq_from_rng};
    }

    TEST(CreateSeedSeqFrom, CompatibleWithRawURBG) {
        // Construct a URBG instance.
        std::random_device urandom;

        // Construct a Seed Sequence from its variates, using 64b of seed-material.
        auto seq_from_rng = abel::CreateSeedSeqFrom(&urandom);

        // Ensure that another URBG can be validly constructed from the Seed Sequence.
        std::mt19937_64{seq_from_rng};
    }

    template<typename URBG>
    void TestReproducibleVariateSequencesForNonsecureURBG() {
        const size_t kNumVariates = 1000;

        // Master RNG instance.
        URBG rng;
        // Reused for both RNG instances.
        auto reusable_seed = abel::CreateSeedSeqFrom(&rng);

        typename URBG::result_type variates[kNumVariates];
        {
            URBG child(reusable_seed);
            for (auto &variate : variates) {
                variate = child();
            }
        }
        // Ensure that variate-sequence can be "replayed" by identical RNG.
        {
            URBG child(reusable_seed);
            for (auto &variate : variates) {
                ASSERT_EQ(variate, child());
            }
        }
    }

    TEST(CreateSeedSeqFrom, ReproducesVariateSequencesForInsecureBitGen) {
        TestReproducibleVariateSequencesForNonsecureURBG<abel::insecure_bit_gen>();
    }

    TEST(CreateSeedSeqFrom, ReproducesVariateSequencesForBitGenerator) {
        TestReproducibleVariateSequencesForNonsecureURBG<abel::bit_gen>();
    }
}  // namespace
