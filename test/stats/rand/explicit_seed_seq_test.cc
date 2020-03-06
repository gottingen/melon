//

#include <abel/stats/random/seed/explicit_seed_seq.h>

#include <iterator>
#include <random>
#include <utility>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <abel/stats/random/seed_sequences.h>

namespace {

    template<typename Sseq>
    bool ConformsToInterface() {
        // Check that the SeedSequence can be default-constructed.
        { Sseq default_constructed_seq; }
        // Check that the SeedSequence can be constructed with two iterators.
        {
            uint32_t init_array[] = {1, 3, 5, 7, 9};
            Sseq iterator_constructed_seq(init_array, &init_array[5]);
        }
        // Check that the SeedSequence can be std::initializer_list-constructed.
        { Sseq list_constructed_seq = {1, 3, 5, 7, 9, 11, 13}; }
        // Check that param() and size() return state provided to constructor.
        {
            uint32_t init_array[] = {1, 2, 3, 4, 5};
            Sseq seq(init_array, &init_array[ABEL_ARRAYSIZE(init_array)]);
            EXPECT_EQ(seq.size(), ABEL_ARRAYSIZE(init_array));

            uint32_t state_array[ABEL_ARRAYSIZE(init_array)];
            seq.param(state_array);

            for (size_t i = 0; i < ABEL_ARRAYSIZE(state_array); i++) {
                EXPECT_EQ(state_array[i], i + 1);
            }
        }
        // Check for presence of generate() method.
        {
            Sseq seq;
            uint32_t seeds[5];

            seq.generate(seeds, &seeds[ABEL_ARRAYSIZE(seeds)]);
        }
        return true;
    }
}  // namespace

TEST(SeedSequences, CheckInterfaces) {
    // Control case
    EXPECT_TRUE(ConformsToInterface<std::seed_seq>());

    // abel classes
    EXPECT_TRUE(ConformsToInterface<abel::random_internal::explicit_seed_seq>());
}

TEST(explicit_seed_seq, DefaultConstructorGeneratesZeros) {
    const size_t kNumBlocks = 128;

    uint32_t outputs[kNumBlocks];
    abel::random_internal::explicit_seed_seq seq;
    seq.generate(outputs, &outputs[kNumBlocks]);

    for (uint32_t &seed : outputs) {
        EXPECT_EQ(seed, 0);
    }
}

TEST(ExplicitSeeqSeq, SeedMaterialIsForwardedIdentically) {
    const size_t kNumBlocks = 128;

    uint32_t seed_material[kNumBlocks];
    std::random_device urandom{"/dev/urandom"};
    for (uint32_t &seed : seed_material) {
        seed = urandom();
    }
    abel::random_internal::explicit_seed_seq seq(seed_material,
                                               &seed_material[kNumBlocks]);

    // Check that output is same as seed-material provided to constructor.
    {
        const size_t kNumGenerated = kNumBlocks / 2;
        uint32_t outputs[kNumGenerated];
        seq.generate(outputs, &outputs[kNumGenerated]);
        for (size_t i = 0; i < kNumGenerated; i++) {
            EXPECT_EQ(outputs[i], seed_material[i]);
        }
    }
    // Check that SeedSequence is stateless between invocations: Despite the last
    // invocation of generate() only consuming half of the input-entropy, the same
    // entropy will be recycled for the next invocation.
    {
        const size_t kNumGenerated = kNumBlocks;
        uint32_t outputs[kNumGenerated];
        seq.generate(outputs, &outputs[kNumGenerated]);
        for (size_t i = 0; i < kNumGenerated; i++) {
            EXPECT_EQ(outputs[i], seed_material[i]);
        }
    }
    // Check that when more seed-material is asked for than is provided, nonzero
    // values are still written.
    {
        const size_t kNumGenerated = kNumBlocks * 2;
        uint32_t outputs[kNumGenerated];
        seq.generate(outputs, &outputs[kNumGenerated]);
        for (size_t i = 0; i < kNumGenerated; i++) {
            EXPECT_EQ(outputs[i], seed_material[i % kNumBlocks]);
        }
    }
}

TEST(explicit_seed_seq, CopyAndMoveConstructors) {
    using testing::Each;
    using testing::Eq;
    using testing::Not;
    using testing::Pointwise;

    uint32_t entropy[4];
    std::random_device urandom("/dev/urandom");
    for (uint32_t &entry : entropy) {
        entry = urandom();
    }
    abel::random_internal::explicit_seed_seq seq_from_entropy(std::begin(entropy),
                                                            std::end(entropy));
    // Copy constructor.
    {
        abel::random_internal::explicit_seed_seq seq_copy(seq_from_entropy);
        EXPECT_EQ(seq_copy.size(), seq_from_entropy.size());

        std::vector<uint32_t> seeds_1;
        seeds_1.resize(1000, 0);
        std::vector<uint32_t> seeds_2;
        seeds_2.resize(1000, 1);

        seq_from_entropy.generate(seeds_1.begin(), seeds_1.end());
        seq_copy.generate(seeds_2.begin(), seeds_2.end());

        EXPECT_THAT(seeds_1, Pointwise(Eq(), seeds_2));
    }
    // Assignment operator.
    {
        for (uint32_t &entry : entropy) {
            entry = urandom();
        }
        abel::random_internal::explicit_seed_seq another_seq(std::begin(entropy),
                                                           std::end(entropy));

        std::vector<uint32_t> seeds_1;
        seeds_1.resize(1000, 0);
        std::vector<uint32_t> seeds_2;
        seeds_2.resize(1000, 0);

        seq_from_entropy.generate(seeds_1.begin(), seeds_1.end());
        another_seq.generate(seeds_2.begin(), seeds_2.end());

        // Assert precondition: Sequences generated by seed-sequences are not equal.
        EXPECT_THAT(seeds_1, Not(Pointwise(Eq(), seeds_2)));

        // Apply the assignment-operator.
        another_seq = seq_from_entropy;

        // Re-generate seeds.
        seq_from_entropy.generate(seeds_1.begin(), seeds_1.end());
        another_seq.generate(seeds_2.begin(), seeds_2.end());

        // Seeds generated by seed-sequences should now be equal.
        EXPECT_THAT(seeds_1, Pointwise(Eq(), seeds_2));
    }
    // Move constructor.
    {
        // Get seeds from seed-sequence constructed from entropy.
        std::vector<uint32_t> seeds_1;
        seeds_1.resize(1000, 0);
        seq_from_entropy.generate(seeds_1.begin(), seeds_1.end());

        // Apply move-constructor move the sequence to another instance.
        abel::random_internal::explicit_seed_seq moved_seq(
                std::move(seq_from_entropy));
        std::vector<uint32_t> seeds_2;
        seeds_2.resize(1000, 1);
        moved_seq.generate(seeds_2.begin(), seeds_2.end());
        // Verify that seeds produced by moved-instance are the same as original.
        EXPECT_THAT(seeds_1, Pointwise(Eq(), seeds_2));

        // Verify that the moved-from instance now behaves like a
        // default-constructed instance.
        EXPECT_EQ(seq_from_entropy.size(), 0);
        seq_from_entropy.generate(seeds_1.begin(), seeds_1.end());
        EXPECT_THAT(seeds_1, Each(Eq(0)));
    }
}
