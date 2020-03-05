//

#include <abel/stats/random/seed/salted_seed_seq.h>

#include <iterator>
#include <random>
#include <utility>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using abel::random_internal::get_salt_material;
using abel::random_internal::make_salted_seed_seq;
using abel::random_internal::salted_seed_seq;
using testing::Eq;
using testing::Pointwise;

namespace {

    template<typename Sseq>
    void ConformsToInterface() {
        // Check that the SeedSequence can be default-constructed.
        { Sseq default_constructed_seq; }
        // Check that the SeedSequence can be constructed with two iterators.
        {
            uint32_t init_array[] = {1, 3, 5, 7, 9};
            Sseq iterator_constructed_seq(std::begin(init_array), std::end(init_array));
        }
        // Check that the SeedSequence can be std::initializer_list-constructed.
        { Sseq list_constructed_seq = {1, 3, 5, 7, 9, 11, 13}; }
        // Check that param() and size() return state provided to constructor.
        {
            uint32_t init_array[] = {1, 2, 3, 4, 5};
            Sseq seq(std::begin(init_array), std::end(init_array));
            EXPECT_EQ(seq.size(), ABEL_ARRAYSIZE(init_array));

            std::vector<uint32_t> state_vector;
            seq.param(std::back_inserter(state_vector));

            EXPECT_EQ(state_vector.size(), ABEL_ARRAYSIZE(init_array));
            for (size_t i = 0; i < state_vector.size(); i++) {
                EXPECT_EQ(state_vector[i], i + 1);
            }
        }
        // Check for presence of generate() method.
        {
            Sseq seq;
            uint32_t seeds[5];

            seq.generate(std::begin(seeds), std::end(seeds));
        }
    }

    TEST(salted_seed_seq, CheckInterfaces) {
        // Control case
        ConformsToInterface<std::seed_seq>();

        // abel classes
        ConformsToInterface<salted_seed_seq<std::seed_seq>>();
    }

    TEST(salted_seed_seq, CheckConstructingFromOtherSequence) {
        std::vector<uint32_t> seed_values(10, 1);
        std::seed_seq seq(seed_values.begin(), seed_values.end());
        auto salted_seq = make_salted_seed_seq(std::move(seq));

        EXPECT_EQ(seq.size(), salted_seq.size());

        std::vector<uint32_t> param_result;
        seq.param(std::back_inserter(param_result));

        EXPECT_EQ(seed_values, param_result);
    }

    TEST(salted_seed_seq, SaltedSaltedSeedSeqIsNotDoubleSalted) {
        uint32_t init[] = {1, 3, 5, 7, 9};

        std::seed_seq seq(std::begin(init), std::end(init));

        // The first salting.
        salted_seed_seq<std::seed_seq> salted_seq = make_salted_seed_seq(std::move(seq));
        uint32_t a[16];
        salted_seq.generate(std::begin(a), std::end(a));

        // The second salting.
        salted_seed_seq<std::seed_seq> salted_salted_seq =
                make_salted_seed_seq(std::move(salted_seq));
        uint32_t b[16];
        salted_salted_seq.generate(std::begin(b), std::end(b));

        // ... both should be equal.
        EXPECT_THAT(b, Pointwise(Eq(), a)) << "a[0] " << a[0];
    }

    TEST(salted_seed_seq, SeedMaterialIsSalted) {
        const size_t kNumBlocks = 16;

        uint32_t seed_material[kNumBlocks];
        std::random_device urandom{"/dev/urandom"};
        for (uint32_t &seed : seed_material) {
            seed = urandom();
        }

        std::seed_seq seq(std::begin(seed_material), std::end(seed_material));
        salted_seed_seq<std::seed_seq> salted_seq(std::begin(seed_material),
                                                std::end(seed_material));

        bool salt_is_available = get_salt_material().has_value();

        // If salt is available generated sequence should be different.
        if (salt_is_available) {
            uint32_t outputs[kNumBlocks];
            uint32_t salted_outputs[kNumBlocks];

            seq.generate(std::begin(outputs), std::end(outputs));
            salted_seq.generate(std::begin(salted_outputs), std::end(salted_outputs));

            EXPECT_THAT(outputs, Pointwise(testing::Ne(), salted_outputs));
        }
    }

    TEST(salted_seed_seq, GenerateAcceptsDifferentTypes) {
        const size_t kNumBlocks = 4;

        salted_seed_seq<std::seed_seq> seq({1, 2, 3});

        uint32_t expected[kNumBlocks];
        seq.generate(std::begin(expected), std::end(expected));

        // 32-bit outputs
        {
            unsigned long seed_material[kNumBlocks];  // NOLINT(runtime/int)
            seq.generate(std::begin(seed_material), std::end(seed_material));
            EXPECT_THAT(seed_material, Pointwise(Eq(), expected));
        }
        {
            unsigned int seed_material[kNumBlocks];  // NOLINT(runtime/int)
            seq.generate(std::begin(seed_material), std::end(seed_material));
            EXPECT_THAT(seed_material, Pointwise(Eq(), expected));
        }

        // 64-bit outputs.
        {
            uint64_t seed_material[kNumBlocks];
            seq.generate(std::begin(seed_material), std::end(seed_material));
            EXPECT_THAT(seed_material, Pointwise(Eq(), expected));
        }
        {
            int64_t seed_material[kNumBlocks];
            seq.generate(std::begin(seed_material), std::end(seed_material));
            EXPECT_THAT(seed_material, Pointwise(Eq(), expected));
        }
    }

}  // namespace
