//

#include <cinttypes>
#include <random>
#include <sstream>
#include <vector>

#include <gtest/gtest.h>
#include <abel/stats/random/random.h>

template<typename T>
void Use(T) {}

TEST(Examples, Basic) {
    abel::bit_gen gen;
    std::vector<int> objs = {10, 20, 30, 40, 50};

    // Choose an element from a set.
    auto elem = objs[abel::uniform(gen, 0u, objs.size())];
    Use(elem);

    // Generate a uniform value between 1 and 6.
    auto dice_roll = abel::uniform<int>(abel::IntervalClosedClosed, gen, 1, 6);
    Use(dice_roll);

    // Generate a random byte.
    auto byte = abel::uniform<uint8_t>(gen);
    Use(byte);

    // Generate a fractional value from [0f, 1f).
    auto fraction = abel::uniform<float>(gen, 0, 1);
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
    // so when using abel::bit_gen it must be an intentional choice.
    // NOTE: All of these only exhibit process-level stability.

    // Create a correlated sequence from system entropy.
    {
        auto my_seed = abel::make_seed_seq();

        abel::bit_gen gen_1(my_seed);
        abel::bit_gen gen_2(my_seed);  // Produces same variates as gen_1.

        EXPECT_EQ(abel::Bernoulli(gen_1, 0.5), abel::Bernoulli(gen_2, 0.5));
        EXPECT_EQ(abel::uniform<uint32_t>(gen_1), abel::uniform<uint32_t>(gen_2));
    }

    // Create a correlated sequence from an existing URBG.
    {
        abel::bit_gen gen;

        auto my_seed = abel::create_seed_seq_from(&gen);
        abel::bit_gen gen_1(my_seed);
        abel::bit_gen gen_2(my_seed);

        EXPECT_EQ(abel::Bernoulli(gen_1, 0.5), abel::Bernoulli(gen_2, 0.5));
        EXPECT_EQ(abel::uniform<uint32_t>(gen_1), abel::uniform<uint32_t>(gen_2));
    }

    // An alternate construction which uses user-supplied data
    // instead of a random seed.
    {
        const char kData[] = "A simple seed string";
        std::seed_seq my_seed(std::begin(kData), std::end(kData));

        abel::bit_gen gen_1(my_seed);
        abel::bit_gen gen_2(my_seed);

        EXPECT_EQ(abel::Bernoulli(gen_1, 0.5), abel::Bernoulli(gen_2, 0.5));
        EXPECT_EQ(abel::uniform<uint32_t>(gen_1), abel::uniform<uint32_t>(gen_2));
    }
}

