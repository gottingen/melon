//

#include <cinttypes>
#include <random>
#include <sstream>
#include <vector>

#include <gtest/gtest.h>
#include <abel/random/random.h>

template<typename T>
void Use(T) {}

TEST(Examples, Basic) {
    abel::BitGen gen;
    std::vector<int> objs = {10, 20, 30, 40, 50};

    // Choose an element from a set.
    auto elem = objs[abel::Uniform(gen, 0u, objs.size())];
    Use(elem);

    // Generate a uniform value between 1 and 6.
    auto dice_roll = abel::Uniform<int>(abel::IntervalClosedClosed, gen, 1, 6);
    Use(dice_roll);

    // Generate a random byte.
    auto byte = abel::Uniform<uint8_t>(gen);
    Use(byte);

    // Generate a fractional value from [0f, 1f).
    auto fraction = abel::Uniform<float>(gen, 0, 1);
    Use(fraction);

    // Toss a fair coin; 50/50 probability.
    bool coin_toss = abel::Bernoulli(gen, 0.5);
    Use(coin_toss);

    // Select a file size between 1k and 10MB, biased towards smaller file sizes.
    auto file_size = abel::LogUniform<size_t>(gen, 1000, 10 * 1000 * 1000);
    Use(file_size);

    // Randomize (shuffle) a collection.
    std::shuffle(std::begin(objs), std::end(objs), gen);
}

TEST(Examples, CreateingCorrelatedVariateSequences) {
    // Unexpected PRNG correlation is often a source of bugs,
    // so when using abel::BitGen it must be an intentional choice.
    // NOTE: All of these only exhibit process-level stability.

    // Create a correlated sequence from system entropy.
    {
        auto my_seed = abel::MakeSeedSeq();

        abel::BitGen gen_1(my_seed);
        abel::BitGen gen_2(my_seed);  // Produces same variates as gen_1.

        EXPECT_EQ(abel::Bernoulli(gen_1, 0.5), abel::Bernoulli(gen_2, 0.5));
        EXPECT_EQ(abel::Uniform<uint32_t>(gen_1), abel::Uniform<uint32_t>(gen_2));
    }

    // Create a correlated sequence from an existing URBG.
    {
        abel::BitGen gen;

        auto my_seed = abel::CreateSeedSeqFrom(&gen);
        abel::BitGen gen_1(my_seed);
        abel::BitGen gen_2(my_seed);

        EXPECT_EQ(abel::Bernoulli(gen_1, 0.5), abel::Bernoulli(gen_2, 0.5));
        EXPECT_EQ(abel::Uniform<uint32_t>(gen_1), abel::Uniform<uint32_t>(gen_2));
    }

    // An alternate construction which uses user-supplied data
    // instead of a random seed.
    {
        const char kData[] = "A simple seed string";
        std::seed_seq my_seed(std::begin(kData), std::end(kData));

        abel::BitGen gen_1(my_seed);
        abel::BitGen gen_2(my_seed);

        EXPECT_EQ(abel::Bernoulli(gen_1, 0.5), abel::Bernoulli(gen_2, 0.5));
        EXPECT_EQ(abel::Uniform<uint32_t>(gen_1), abel::Uniform<uint32_t>(gen_2));
    }
}

