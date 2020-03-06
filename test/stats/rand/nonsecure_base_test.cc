//

#include <abel/stats/random/engine/nonsecure_base.h>

#include <algorithm>
#include <iostream>
#include <memory>
#include <random>
#include <sstream>

#include <gtest/gtest.h>
#include <abel/stats/random/distributions.h>
#include <abel/stats/random/random.h>
#include <abel/strings/str_cat.h>

namespace {

    using ExampleNonsecureURBG =
    abel::random_internal::nonsecure_urgb_base<std::mt19937>;

    template<typename T>
    void Use(const T &) {}

}  // namespace

TEST(nonsecure_urgb_base, DefaultConstructorIsValid) {
    ExampleNonsecureURBG urbg;
}

// Ensure that the recommended template-instantiations are valid.
TEST(RecommendedTemplates, CanBeConstructed) {
    abel::bit_gen default_generator;
    abel::insecure_bit_gen insecure_generator;
}

TEST(RecommendedTemplates, CanDiscardValues) {
    abel::bit_gen default_generator;
    abel::insecure_bit_gen insecure_generator;

    default_generator.discard(5);
    insecure_generator.discard(5);
}

TEST(nonsecure_urgb_base, StandardInterface) {
    // Names after definition of [rand.req.urbg] in C++ standard.
    // e us a value of E
    // v is a lvalue of E
    // x, y are possibly const values of E
    // s is a value of T
    // q is a value satisfying requirements of seed_sequence
    // z is a value of type unsigned long long
    // os is a some specialization of basic_ostream
    // is is a some specialization of basic_istream

    using E = abel::random_internal::nonsecure_urgb_base<std::minstd_rand>;

    using T = typename E::result_type;

    static_assert(!std::is_copy_constructible<E>::value,
                  "nonsecure_urgb_base should not be copy constructible");

    static_assert(!abel::is_copy_assignable<E>::value,
                  "nonsecure_urgb_base should not be copy assignable");

    static_assert(std::is_move_constructible<E>::value,
                  "nonsecure_urgb_base should be move constructible");

    static_assert(abel::is_move_assignable<E>::value,
                  "nonsecure_urgb_base should be move assignable");

    static_assert(std::is_same<decltype(std::declval<E>()()), T>::value,
                  "return type of operator() must be result_type");

    {
        const E x, y;
        Use(x);
        Use(y);

        static_assert(std::is_same<decltype(x == y), bool>::value,
                      "return type of operator== must be bool");

        static_assert(std::is_same<decltype(x != y), bool>::value,
                      "return type of operator== must be bool");
    }

    E e;
    std::seed_seq q{1, 2, 3};

    E{};
    E{q};

    // Copy constructor not supported.
    // E{x};

    // result_type seed constructor not supported.
    // E{T{1}};

    // Move constructors are supported.
    {
        E tmp(q);
        E m = std::move(tmp);
        E n(std::move(m));
        EXPECT_TRUE(e != n);
    }

    // Comparisons work.
    {
        // MSVC emits error 2718 when using EXPECT_EQ(e, x)
        //  * actual parameter with __declspec(align('#')) won't be aligned
        E a(q);
        E b(q);

        EXPECT_TRUE(a != e);
        EXPECT_TRUE(a == b);

        a();
        EXPECT_TRUE(a != b);
    }

    // e.seed(s) not supported.

    // [rand.req.eng] specifies the parameter as 'unsigned long long'
    // e.discard(unsigned long long) is supported.
    unsigned long long z = 1;  // NOLINT(runtime/int)
    e.discard(z);
}

TEST(nonsecure_urgb_base, SeedSeqConstructorIsValid) {
    std::seed_seq seq;
    ExampleNonsecureURBG rbg(seq);
}

TEST(nonsecure_urgb_base, CompatibleWithDistributionUtils) {
    ExampleNonsecureURBG rbg;

    abel::uniform(rbg, 0, 100);
    abel::uniform(rbg, 0.5, 0.7);
    abel::Poisson<uint32_t>(rbg);
    abel::Exponential<float>(rbg);
}

TEST(nonsecure_urgb_base, CompatibleWithStdDistributions) {
    ExampleNonsecureURBG rbg;

    // Cast to void to suppress [[nodiscard]] warnings
    static_cast<void>(std::uniform_int_distribution<uint32_t>(0, 100)(rbg));
    static_cast<void>(std::uniform_real_distribution<float>()(rbg));
    static_cast<void>(std::bernoulli_distribution(0.2)(rbg));
}

TEST(nonsecure_urgb_base, ConsecutiveDefaultInstancesYieldUniqueVariates) {
    const size_t kNumSamples = 128;

    ExampleNonsecureURBG rbg1;
    ExampleNonsecureURBG rbg2;

    for (size_t i = 0; i < kNumSamples; i++) {
        EXPECT_NE(rbg1(), rbg2());
    }
}

TEST(nonsecure_urgb_base, EqualSeedSequencesYieldEqualVariates) {
    std::seed_seq seq;

    ExampleNonsecureURBG rbg1(seq);
    ExampleNonsecureURBG rbg2(seq);

    // ExampleNonsecureURBG rbg3({1, 2, 3});  // Should not compile.

    for (uint32_t i = 0; i < 1000; i++) {
        EXPECT_EQ(rbg1(), rbg2());
    }

    rbg1.discard(100);
    rbg2.discard(100);

    // The sequences should continue after discarding
    for (uint32_t i = 0; i < 1000; i++) {
        EXPECT_EQ(rbg1(), rbg2());
    }
}

// This is a PRNG-compatible type specifically designed to test
// that nonsecure_urgb_base::Seeder can correctly handle iterators
// to arbitrary non-uint32_t size types.
template<typename T>
struct SeederTestEngine {
    using result_type = T;

    static constexpr result_type (min)() {
        return (std::numeric_limits<result_type>::min)();
    }

    static constexpr result_type (max)() {
        return (std::numeric_limits<result_type>::max)();
    }

    template<class SeedSequence,
            typename = typename abel::enable_if_t<
                    !std::is_same<SeedSequence, SeederTestEngine>::value>>
    explicit SeederTestEngine(SeedSequence &&seq) {
        seed(seq);
    }

    SeederTestEngine(const SeederTestEngine &) = default;

    SeederTestEngine &operator=(const SeederTestEngine &) = default;

    SeederTestEngine(SeederTestEngine &&) = default;

    SeederTestEngine &operator=(SeederTestEngine &&) = default;

    result_type operator()() { return state[0]; }

    template<class SeedSequence>
    void seed(SeedSequence &&seq) {
        std::fill(std::begin(state), std::end(state), T(0));
        seq.generate(std::begin(state), std::end(state));
    }

    T state[2];
};

TEST(nonsecure_urgb_base, SeederWorksForU32) {
    using U32 =
    abel::random_internal::nonsecure_urgb_base<SeederTestEngine<uint32_t>>;
    U32 x;
    EXPECT_NE(0, x());
}

TEST(nonsecure_urgb_base, SeederWorksForU64) {
    using U64 =
    abel::random_internal::nonsecure_urgb_base<SeederTestEngine<uint64_t>>;

    U64 x;
    EXPECT_NE(0, x());
}
