//

#include <abel/stats/random/distributions.h>

#include <cmath>
#include <cstdint>
#include <random>
#include <vector>

#include <gtest/gtest.h>
#include <test/testing/distribution_test_util.h>
#include <abel/stats/random/random.h>

namespace {

    constexpr int kSize = 400000;

    class RandomDistributionsTest : public testing::Test {
    };

    TEST_F(RandomDistributionsTest, UniformBoundFunctions) {
        using abel::IntervalClosedClosed;
        using abel::IntervalClosedOpen;
        using abel::IntervalOpenClosed;
        using abel::IntervalOpenOpen;
        using abel::random_internal::uniform_lower_bound;
        using abel::random_internal::uniform_upper_bound;

        // abel::uniform_int_distribution natively assumes IntervalClosedClosed
        // abel::uniform_real_distribution natively assumes IntervalClosedOpen

        EXPECT_EQ(uniform_lower_bound(IntervalOpenClosed, 0, 100), 1);
        EXPECT_EQ(uniform_lower_bound(IntervalOpenOpen, 0, 100), 1);
        EXPECT_GT(uniform_lower_bound<float>(IntervalOpenClosed, 0, 1.0), 0);
        EXPECT_GT(uniform_lower_bound<float>(IntervalOpenOpen, 0, 1.0), 0);
        EXPECT_GT(uniform_lower_bound<double>(IntervalOpenClosed, 0, 1.0), 0);
        EXPECT_GT(uniform_lower_bound<double>(IntervalOpenOpen, 0, 1.0), 0);

        EXPECT_EQ(uniform_lower_bound(IntervalClosedClosed, 0, 100), 0);
        EXPECT_EQ(uniform_lower_bound(IntervalClosedOpen, 0, 100), 0);
        EXPECT_EQ(uniform_lower_bound<float>(IntervalClosedClosed, 0, 1.0), 0);
        EXPECT_EQ(uniform_lower_bound<float>(IntervalClosedOpen, 0, 1.0), 0);
        EXPECT_EQ(uniform_lower_bound<double>(IntervalClosedClosed, 0, 1.0), 0);
        EXPECT_EQ(uniform_lower_bound<double>(IntervalClosedOpen, 0, 1.0), 0);

        EXPECT_EQ(uniform_upper_bound(IntervalOpenOpen, 0, 100), 99);
        EXPECT_EQ(uniform_upper_bound(IntervalClosedOpen, 0, 100), 99);
        EXPECT_EQ(uniform_upper_bound<float>(IntervalOpenOpen, 0, 1.0), 1.0);
        EXPECT_EQ(uniform_upper_bound<float>(IntervalClosedOpen, 0, 1.0), 1.0);
        EXPECT_EQ(uniform_upper_bound<double>(IntervalOpenOpen, 0, 1.0), 1.0);
        EXPECT_EQ(uniform_upper_bound<double>(IntervalClosedOpen, 0, 1.0), 1.0);

        EXPECT_EQ(uniform_upper_bound(IntervalOpenClosed, 0, 100), 100);
        EXPECT_EQ(uniform_upper_bound(IntervalClosedClosed, 0, 100), 100);
        EXPECT_GT(uniform_upper_bound<float>(IntervalOpenClosed, 0, 1.0), 1.0);
        EXPECT_GT(uniform_upper_bound<float>(IntervalClosedClosed, 0, 1.0), 1.0);
        EXPECT_GT(uniform_upper_bound<double>(IntervalOpenClosed, 0, 1.0), 1.0);
        EXPECT_GT(uniform_upper_bound<double>(IntervalClosedClosed, 0, 1.0), 1.0);

        // Negative value tests
        EXPECT_EQ(uniform_lower_bound(IntervalOpenClosed, -100, -1), -99);
        EXPECT_EQ(uniform_lower_bound(IntervalOpenOpen, -100, -1), -99);
        EXPECT_GT(uniform_lower_bound<float>(IntervalOpenClosed, -2.0, -1.0), -2.0);
        EXPECT_GT(uniform_lower_bound<float>(IntervalOpenOpen, -2.0, -1.0), -2.0);
        EXPECT_GT(uniform_lower_bound<double>(IntervalOpenClosed, -2.0, -1.0), -2.0);
        EXPECT_GT(uniform_lower_bound<double>(IntervalOpenOpen, -2.0, -1.0), -2.0);

        EXPECT_EQ(uniform_lower_bound(IntervalClosedClosed, -100, -1), -100);
        EXPECT_EQ(uniform_lower_bound(IntervalClosedOpen, -100, -1), -100);
        EXPECT_EQ(uniform_lower_bound<float>(IntervalClosedClosed, -2.0, -1.0), -2.0);
        EXPECT_EQ(uniform_lower_bound<float>(IntervalClosedOpen, -2.0, -1.0), -2.0);
        EXPECT_EQ(uniform_lower_bound<double>(IntervalClosedClosed, -2.0, -1.0),
                  -2.0);
        EXPECT_EQ(uniform_lower_bound<double>(IntervalClosedOpen, -2.0, -1.0), -2.0);

        EXPECT_EQ(uniform_upper_bound(IntervalOpenOpen, -100, -1), -2);
        EXPECT_EQ(uniform_upper_bound(IntervalClosedOpen, -100, -1), -2);
        EXPECT_EQ(uniform_upper_bound<float>(IntervalOpenOpen, -2.0, -1.0), -1.0);
        EXPECT_EQ(uniform_upper_bound<float>(IntervalClosedOpen, -2.0, -1.0), -1.0);
        EXPECT_EQ(uniform_upper_bound<double>(IntervalOpenOpen, -2.0, -1.0), -1.0);
        EXPECT_EQ(uniform_upper_bound<double>(IntervalClosedOpen, -2.0, -1.0), -1.0);

        EXPECT_EQ(uniform_upper_bound(IntervalOpenClosed, -100, -1), -1);
        EXPECT_EQ(uniform_upper_bound(IntervalClosedClosed, -100, -1), -1);
        EXPECT_GT(uniform_upper_bound<float>(IntervalOpenClosed, -2.0, -1.0), -1.0);
        EXPECT_GT(uniform_upper_bound<float>(IntervalClosedClosed, -2.0, -1.0), -1.0);
        EXPECT_GT(uniform_upper_bound<double>(IntervalOpenClosed, -2.0, -1.0), -1.0);
        EXPECT_GT(uniform_upper_bound<double>(IntervalClosedClosed, -2.0, -1.0),
                  -1.0);

        // Edge cases: the next value toward itself is itself.
        const double d = 1.0;
        const float f = 1.0;
        EXPECT_EQ(uniform_lower_bound(IntervalOpenClosed, d, d), d);
        EXPECT_EQ(uniform_lower_bound(IntervalOpenClosed, f, f), f);

        EXPECT_GT(uniform_lower_bound(IntervalOpenClosed, 1.0, 2.0), 1.0);
        EXPECT_LT(uniform_lower_bound(IntervalOpenClosed, 1.0, +0.0), 1.0);
        EXPECT_LT(uniform_lower_bound(IntervalOpenClosed, 1.0, -0.0), 1.0);
        EXPECT_LT(uniform_lower_bound(IntervalOpenClosed, 1.0, -1.0), 1.0);

        EXPECT_EQ(uniform_upper_bound(IntervalClosedClosed, 0.0f,
                                      std::numeric_limits<float>::max()),
                  std::numeric_limits<float>::max());
        EXPECT_EQ(uniform_upper_bound(IntervalClosedClosed, 0.0,
                                      std::numeric_limits<double>::max()),
                  std::numeric_limits<double>::max());
    }

    struct Invalid {
    };

    template<typename A, typename B>
    auto InferredUniformReturnT(int)
    -> decltype(abel::uniform(std::declval<abel::insecure_bit_gen &>(),
                              std::declval<A>(), std::declval<B>()));

    template<typename, typename>
    Invalid InferredUniformReturnT(...);

    template<typename TagType, typename A, typename B>
    auto InferredTaggedUniformReturnT(int)
    -> decltype(abel::uniform(std::declval<TagType>(),
                              std::declval<abel::insecure_bit_gen &>(),
                              std::declval<A>(), std::declval<B>()));

    template<typename, typename, typename>
    Invalid InferredTaggedUniformReturnT(...);

// Given types <A, B, Expect>, CheckArgsInferType() verifies that
//
//   abel::uniform(gen, A{}, B{})
//
// returns the type "Expect".
//
// This interface can also be used to assert that a given abel::uniform()
// overload does not exist / will not compile. Given types <A, B>, the
// expression
//
//   decltype(abel::uniform(..., std::declval<A>(), std::declval<B>()))
//
// will not compile, leaving the definition of InferredUniformReturnT<A, B> to
// resolve (via SFINAE) to the overload which returns type "Invalid". This
// allows tests to assert that an invocation such as
//
//   abel::uniform(gen, 1.23f, std::numeric_limits<int>::max() - 1)
//
// should not compile, since neither type, float nor int, can precisely
// represent both endpoint-values. Writing:
//
//   CheckArgsInferType<float, int, Invalid>()
//
// will assert that this overload does not exist.
    template<typename A, typename B, typename Expect>
    void CheckArgsInferType() {
        static_assert(
                abel::conjunction<
                        std::is_same<Expect, decltype(InferredUniformReturnT<A, B>(0))>,
                        std::is_same<Expect,
                                decltype(InferredUniformReturnT<B, A>(0))>>::value,
                "");
        static_assert(
                abel::conjunction<
                        std::is_same<Expect, decltype(InferredTaggedUniformReturnT<
                                abel::IntervalOpenOpenTag, A, B>(0))>,
                        std::is_same<Expect,
                                decltype(InferredTaggedUniformReturnT<
                                        abel::IntervalOpenOpenTag, B, A>(0))>>::value,
                "");
    }

    template<typename A, typename B, typename ExplicitRet>
    auto ExplicitUniformReturnT(int) -> decltype(
    abel::uniform<ExplicitRet>(*std::declval<abel::insecure_bit_gen *>(),
                               std::declval<A>(), std::declval<B>()));

    template<typename, typename, typename ExplicitRet>
    Invalid ExplicitUniformReturnT(...);

    template<typename TagType, typename A, typename B, typename ExplicitRet>
    auto ExplicitTaggedUniformReturnT(int) -> decltype(abel::uniform<ExplicitRet>(
            std::declval<TagType>(), *std::declval<abel::insecure_bit_gen *>(),
            std::declval<A>(), std::declval<B>()));

    template<typename, typename, typename, typename ExplicitRet>
    Invalid ExplicitTaggedUniformReturnT(...);

// Given types <A, B, Expect>, CheckArgsReturnExpectedType() verifies that
//
//   abel::uniform<Expect>(gen, A{}, B{})
//
// returns the type "Expect", and that the function-overload has the signature
//
//   Expect(URBG&, Expect, Expect)
    template<typename A, typename B, typename Expect>
    void CheckArgsReturnExpectedType() {
        static_assert(
                abel::conjunction<
                        std::is_same<Expect,
                                decltype(ExplicitUniformReturnT<A, B, Expect>(0))>,
                        std::is_same<Expect, decltype(ExplicitUniformReturnT<B, A, Expect>(
                                0))>>::value,
                "");
        static_assert(
                abel::conjunction<
                        std::is_same<Expect,
                                decltype(ExplicitTaggedUniformReturnT<
                                        abel::IntervalOpenOpenTag, A, B, Expect>(0))>,
                        std::is_same<Expect, decltype(ExplicitTaggedUniformReturnT<
                                abel::IntervalOpenOpenTag, B, A,
                                Expect>(0))>>::value,
                "");
    }

    TEST_F(RandomDistributionsTest, UniformTypeInference) {
        // Infers common types.
        CheckArgsInferType<uint16_t, uint16_t, uint16_t>();
        CheckArgsInferType<uint32_t, uint32_t, uint32_t>();
        CheckArgsInferType<uint64_t, uint64_t, uint64_t>();
        CheckArgsInferType<int16_t, int16_t, int16_t>();
        CheckArgsInferType<int32_t, int32_t, int32_t>();
        CheckArgsInferType<int64_t, int64_t, int64_t>();
        CheckArgsInferType<float, float, float>();
        CheckArgsInferType<double, double, double>();

        // Explicitly-specified return-values override inferences.
        CheckArgsReturnExpectedType<int16_t, int16_t, int32_t>();
        CheckArgsReturnExpectedType<uint16_t, uint16_t, int32_t>();
        CheckArgsReturnExpectedType<int16_t, int16_t, int64_t>();
        CheckArgsReturnExpectedType<int16_t, int32_t, int64_t>();
        CheckArgsReturnExpectedType<int16_t, int32_t, double>();
        CheckArgsReturnExpectedType<float, float, double>();
        CheckArgsReturnExpectedType<int, int, int16_t>();

        // Properly promotes uint16_t.
        CheckArgsInferType<uint16_t, uint32_t, uint32_t>();
        CheckArgsInferType<uint16_t, uint64_t, uint64_t>();
        CheckArgsInferType<uint16_t, int32_t, int32_t>();
        CheckArgsInferType<uint16_t, int64_t, int64_t>();
        CheckArgsInferType<uint16_t, float, float>();
        CheckArgsInferType<uint16_t, double, double>();

        // Properly promotes int16_t.
        CheckArgsInferType<int16_t, int32_t, int32_t>();
        CheckArgsInferType<int16_t, int64_t, int64_t>();
        CheckArgsInferType<int16_t, float, float>();
        CheckArgsInferType<int16_t, double, double>();

        // Invalid (u)int16_t-pairings do not compile.
        // See "CheckArgsInferType" comments above, for how this is achieved.
        CheckArgsInferType<uint16_t, int16_t, Invalid>();
        CheckArgsInferType<int16_t, uint32_t, Invalid>();
        CheckArgsInferType<int16_t, uint64_t, Invalid>();

        // Properly promotes uint32_t.
        CheckArgsInferType<uint32_t, uint64_t, uint64_t>();
        CheckArgsInferType<uint32_t, int64_t, int64_t>();
        CheckArgsInferType<uint32_t, double, double>();

        // Properly promotes int32_t.
        CheckArgsInferType<int32_t, int64_t, int64_t>();
        CheckArgsInferType<int32_t, double, double>();

        // Invalid (u)int32_t-pairings do not compile.
        CheckArgsInferType<uint32_t, int32_t, Invalid>();
        CheckArgsInferType<int32_t, uint64_t, Invalid>();
        CheckArgsInferType<int32_t, float, Invalid>();
        CheckArgsInferType<uint32_t, float, Invalid>();

        // Invalid (u)int64_t-pairings do not compile.
        CheckArgsInferType<uint64_t, int64_t, Invalid>();
        CheckArgsInferType<int64_t, float, Invalid>();
        CheckArgsInferType<int64_t, double, Invalid>();

        // Properly promotes float.
        CheckArgsInferType<float, double, double>();

        // Examples.
        abel::insecure_bit_gen gen;
        EXPECT_NE(1, abel::uniform(gen, static_cast<uint16_t>(0), 1.0f));
        EXPECT_NE(1, abel::uniform(gen, 0, 1.0));
        EXPECT_NE(1, abel::uniform(abel::IntervalOpenOpen, gen,
                                   static_cast<uint16_t>(0), 1.0f));
        EXPECT_NE(1, abel::uniform(abel::IntervalOpenOpen, gen, 0, 1.0));
        EXPECT_NE(1, abel::uniform(abel::IntervalOpenOpen, gen, -1, 1.0));
        EXPECT_NE(1, abel::uniform<double>(abel::IntervalOpenOpen, gen, -1, 1));
        EXPECT_NE(1, abel::uniform<float>(abel::IntervalOpenOpen, gen, 0, 1));
        EXPECT_NE(1, abel::uniform<float>(gen, 0, 1));
    }

    TEST_F(RandomDistributionsTest, UniformNoBounds) {
        abel::insecure_bit_gen gen;

        abel::uniform<uint8_t>(gen);
        abel::uniform<uint16_t>(gen);
        abel::uniform<uint32_t>(gen);
        abel::uniform<uint64_t>(gen);
    }

// TODO(lar): Validate properties of non-default interval-semantics.
    TEST_F(RandomDistributionsTest, UniformReal) {
        std::vector<double> values(kSize);

        abel::insecure_bit_gen gen;
        for (int i = 0; i < kSize; i++) {
            values[i] = abel::uniform(gen, 0, 1.0);
        }

        const auto moments =
                abel::random_internal::ComputeDistributionMoments(values);
        EXPECT_NEAR(0.5, moments.mean, 0.02);
        EXPECT_NEAR(1 / 12.0, moments.variance, 0.02);
        EXPECT_NEAR(0.0, moments.skewness, 0.02);
        EXPECT_NEAR(9 / 5.0, moments.kurtosis, 0.02);
    }

    TEST_F(RandomDistributionsTest, UniformInt) {
        std::vector<double> values(kSize);

        abel::insecure_bit_gen gen;
        for (int i = 0; i < kSize; i++) {
            const int64_t kMax = 1000000000000ll;
            int64_t j = abel::uniform(abel::IntervalClosedClosed, gen, 0, kMax);
            // convert to double.
            values[i] = static_cast<double>(j) / static_cast<double>(kMax);
        }

        const auto moments =
                abel::random_internal::ComputeDistributionMoments(values);
        EXPECT_NEAR(0.5, moments.mean, 0.02);
        EXPECT_NEAR(1 / 12.0, moments.variance, 0.02);
        EXPECT_NEAR(0.0, moments.skewness, 0.02);
        EXPECT_NEAR(9 / 5.0, moments.kurtosis, 0.02);

        /*
        // NOTE: These are not supported by abel::uniform, which is specialized
        // on integer and real valued types.

        enum E { E0, E1 };    // enum
        enum S : int { S0, S1 };    // signed enum
        enum U : unsigned int { U0, U1 };  // unsigned enum

        abel::uniform(gen, E0, E1);
        abel::uniform(gen, S0, S1);
        abel::uniform(gen, U0, U1);
        */
    }

    TEST_F(RandomDistributionsTest, Exponential) {
        std::vector<double> values(kSize);

        abel::insecure_bit_gen gen;
        for (int i = 0; i < kSize; i++) {
            values[i] = abel::Exponential<double>(gen);
        }

        const auto moments =
                abel::random_internal::ComputeDistributionMoments(values);
        EXPECT_NEAR(1.0, moments.mean, 0.02);
        EXPECT_NEAR(1.0, moments.variance, 0.025);
        EXPECT_NEAR(2.0, moments.skewness, 0.1);
        EXPECT_LT(5.0, moments.kurtosis);
    }

    TEST_F(RandomDistributionsTest, PoissonDefault) {
        std::vector<double> values(kSize);

        abel::insecure_bit_gen gen;
        for (int i = 0; i < kSize; i++) {
            values[i] = abel::Poisson<int64_t>(gen);
        }

        const auto moments =
                abel::random_internal::ComputeDistributionMoments(values);
        EXPECT_NEAR(1.0, moments.mean, 0.02);
        EXPECT_NEAR(1.0, moments.variance, 0.02);
        EXPECT_NEAR(1.0, moments.skewness, 0.025);
        EXPECT_LT(2.0, moments.kurtosis);
    }

    TEST_F(RandomDistributionsTest, PoissonLarge) {
        constexpr double kMean = 100000000.0;
        std::vector<double> values(kSize);

        abel::insecure_bit_gen gen;
        for (int i = 0; i < kSize; i++) {
            values[i] = abel::Poisson<int64_t>(gen, kMean);
        }

        const auto moments =
                abel::random_internal::ComputeDistributionMoments(values);
        EXPECT_NEAR(kMean, moments.mean, kMean * 0.015);
        EXPECT_NEAR(kMean, moments.variance, kMean * 0.015);
        EXPECT_NEAR(std::sqrt(kMean), moments.skewness, kMean * 0.02);
        EXPECT_LT(2.0, moments.kurtosis);
    }

    TEST_F(RandomDistributionsTest, Bernoulli) {
        constexpr double kP = 0.5151515151;
        std::vector<double> values(kSize);

        abel::insecure_bit_gen gen;
        for (int i = 0; i < kSize; i++) {
            values[i] = abel::Bernoulli(gen, kP);
        }

        const auto moments =
                abel::random_internal::ComputeDistributionMoments(values);
        EXPECT_NEAR(kP, moments.mean, 0.01);
    }

    TEST_F(RandomDistributionsTest, Beta) {
        constexpr double kAlpha = 2.0;
        constexpr double kBeta = 3.0;
        std::vector<double> values(kSize);

        abel::insecure_bit_gen gen;
        for (int i = 0; i < kSize; i++) {
            values[i] = abel::Beta(gen, kAlpha, kBeta);
        }

        const auto moments =
                abel::random_internal::ComputeDistributionMoments(values);
        EXPECT_NEAR(0.4, moments.mean, 0.01);
    }

    TEST_F(RandomDistributionsTest, Zipf) {
        std::vector<double> values(kSize);

        abel::insecure_bit_gen gen;
        for (int i = 0; i < kSize; i++) {
            values[i] = abel::Zipf<int64_t>(gen, 100);
        }

        // The mean of a zipf distribution is: H(N, s-1) / H(N,s).
        // Given the parameter v = 1, this gives the following function:
        // (Hn(100, 1) - Hn(1,1)) / (Hn(100,2) - Hn(1,2)) = 6.5944
        const auto moments =
                abel::random_internal::ComputeDistributionMoments(values);
        EXPECT_NEAR(6.5944, moments.mean, 2000) << moments;
    }

    TEST_F(RandomDistributionsTest, Gaussian) {
        std::vector<double> values(kSize);

        abel::insecure_bit_gen gen;
        for (int i = 0; i < kSize; i++) {
            values[i] = abel::Gaussian<double>(gen);
        }

        const auto moments =
                abel::random_internal::ComputeDistributionMoments(values);
        EXPECT_NEAR(0.0, moments.mean, 0.02);
        EXPECT_NEAR(1.0, moments.variance, 0.04);
        EXPECT_NEAR(0, moments.skewness, 0.2);
        EXPECT_NEAR(3.0, moments.kurtosis, 0.5);
    }

    TEST_F(RandomDistributionsTest, LogUniform) {
        std::vector<double> values(kSize);

        abel::insecure_bit_gen gen;
        for (int i = 0; i < kSize; i++) {
            values[i] = abel::LogUniform<int64_t>(gen, 0, (1 << 10) - 1);
        }

        // The mean is the sum of the fractional means of the uniform distributions:
        // [0..0][1..1][2..3][4..7][8..15][16..31][32..63]
        // [64..127][128..255][256..511][512..1023]
        const double mean = (0 + 1 + 1 + 2 + 3 + 4 + 7 + 8 + 15 + 16 + 31 + 32 + 63 +
                             64 + 127 + 128 + 255 + 256 + 511 + 512 + 1023) /
                            (2.0 * 11.0);

        const auto moments =
                abel::random_internal::ComputeDistributionMoments(values);
        EXPECT_NEAR(mean, moments.mean, 2) << moments;
    }

}  // namespace
