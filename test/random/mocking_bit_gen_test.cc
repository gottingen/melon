//
//
#include <testing/mocking_bit_gen.h>

#include <numeric>
#include <random>

#include <gmock/gmock.h>
#include <gtest/gtest-spi.h>
#include <gtest/gtest.h>
#include <testing/bit_gen_ref.h>
#include <testing/mock_distributions.h>
#include <abel/random/random.h>

namespace {
    using ::testing::Ne;
    using ::testing::Return;

    TEST(BasicMocking, AllDistributionsAreOverridable) {
        abel::MockingBitGen gen;

        EXPECT_NE(abel::Uniform<int>(gen, 1, 1000000), 20);
        EXPECT_CALL(abel::MockUniform<int>(), Call(gen, 1, 1000000))
        .WillOnce(Return(20));
        EXPECT_EQ(abel::Uniform<int>(gen, 1, 1000000), 20);

        EXPECT_NE(abel::Uniform<double>(gen, 0.0, 100.0), 5.0);
        EXPECT_CALL(abel::MockUniform<double>(), Call(gen, 0.0, 100.0))
        .WillOnce(Return(5.0));
        EXPECT_EQ(abel::Uniform<double>(gen, 0.0, 100.0), 5.0);

        EXPECT_NE(abel::Exponential<double>(gen, 1.0), 42);
        EXPECT_CALL(abel::MockExponential<double>(), Call(gen, 1.0))
        .WillOnce(Return(42));
        EXPECT_EQ(abel::Exponential<double>(gen, 1.0), 42);

        EXPECT_NE(abel::Poisson<int>(gen, 1.0), 500);
        EXPECT_CALL(abel::MockPoisson<int>(), Call(gen, 1.0)).WillOnce(Return(500));
        EXPECT_EQ(abel::Poisson<int>(gen, 1.0), 500);

        EXPECT_NE(abel::Bernoulli(gen, 0.000001), true);
        EXPECT_CALL(abel::MockBernoulli(), Call(gen, 0.000001))
        .WillOnce(Return(true));
        EXPECT_EQ(abel::Bernoulli(gen, 0.000001), true);

        EXPECT_NE(abel::Zipf<int>(gen, 1000000, 2.0, 1.0), 1221);
        EXPECT_CALL(abel::MockZipf<int>(), Call(gen, 1000000, 2.0, 1.0))
        .WillOnce(Return(1221));
        EXPECT_EQ(abel::Zipf<int>(gen, 1000000, 2.0, 1.0), 1221);

        EXPECT_NE(abel::Gaussian<double>(gen, 0.0, 1.0), 0.001);
        EXPECT_CALL(abel::MockGaussian<double>(), Call(gen, 0.0, 1.0))
        .WillOnce(Return(0.001));
        EXPECT_EQ(abel::Gaussian<double>(gen, 0.0, 1.0), 0.001);

        EXPECT_NE(abel::LogUniform<int>(gen, 0, 1000000, 2), 500000);
        EXPECT_CALL(abel::MockLogUniform<int>(), Call(gen, 0, 1000000, 2))
        .WillOnce(Return(500000));
        EXPECT_EQ(abel::LogUniform<int>(gen, 0, 1000000, 2), 500000);
    }

    TEST(BasicMocking, OnDistribution) {
        abel::MockingBitGen gen;

        EXPECT_NE(abel::Uniform<int>(gen, 1, 1000000), 20);
        ON_CALL(abel::MockUniform<int>(), Call(gen, 1, 1000000))
        .WillByDefault(Return(20));
        EXPECT_EQ(abel::Uniform<int>(gen, 1, 1000000), 20);

        EXPECT_NE(abel::Uniform<double>(gen, 0.0, 100.0), 5.0);
        ON_CALL(abel::MockUniform<double>(), Call(gen, 0.0, 100.0))
        .WillByDefault(Return(5.0));
        EXPECT_EQ(abel::Uniform<double>(gen, 0.0, 100.0), 5.0);

        EXPECT_NE(abel::Exponential<double>(gen, 1.0), 42);
        ON_CALL(abel::MockExponential<double>(), Call(gen, 1.0))
        .WillByDefault(Return(42));
        EXPECT_EQ(abel::Exponential<double>(gen, 1.0), 42);

        EXPECT_NE(abel::Poisson<int>(gen, 1.0), 500);
        ON_CALL(abel::MockPoisson<int>(), Call(gen, 1.0)).WillByDefault(Return(500));
        EXPECT_EQ(abel::Poisson<int>(gen, 1.0), 500);

        EXPECT_NE(abel::Bernoulli(gen, 0.000001), true);
        ON_CALL(abel::MockBernoulli(), Call(gen, 0.000001))
        .WillByDefault(Return(true));
        EXPECT_EQ(abel::Bernoulli(gen, 0.000001), true);

        EXPECT_NE(abel::Zipf<int>(gen, 1000000, 2.0, 1.0), 1221);
        ON_CALL(abel::MockZipf<int>(), Call(gen, 1000000, 2.0, 1.0))
        .WillByDefault(Return(1221));
        EXPECT_EQ(abel::Zipf<int>(gen, 1000000, 2.0, 1.0), 1221);

        EXPECT_NE(abel::Gaussian<double>(gen, 0.0, 1.0), 0.001);
        ON_CALL(abel::MockGaussian<double>(), Call(gen, 0.0, 1.0))
        .WillByDefault(Return(0.001));
        EXPECT_EQ(abel::Gaussian<double>(gen, 0.0, 1.0), 0.001);

        EXPECT_NE(abel::LogUniform<int>(gen, 0, 1000000, 2), 2040);
        ON_CALL(abel::MockLogUniform<int>(), Call(gen, 0, 1000000, 2))
        .WillByDefault(Return(2040));
        EXPECT_EQ(abel::LogUniform<int>(gen, 0, 1000000, 2), 2040);
    }

    TEST(BasicMocking, GMockMatchers) {
        abel::MockingBitGen gen;

        EXPECT_NE(abel::Zipf<int>(gen, 1000000, 2.0, 1.0), 1221);
        ON_CALL(abel::MockZipf<int>(), Call(gen, 1000000, 2.0, 1.0))
        .WillByDefault(Return(1221));
        EXPECT_EQ(abel::Zipf<int>(gen, 1000000, 2.0, 1.0), 1221);
    }

    TEST(BasicMocking, OverridesWithMultipleGMockExpectations) {
        abel::MockingBitGen gen;

        EXPECT_CALL(abel::MockUniform<int>(), Call(gen, 1, 10000))
                .WillOnce(Return(20))
                .WillOnce(Return(40))
                .WillOnce(Return(60));
        EXPECT_EQ(abel::Uniform(gen, 1, 10000), 20);
        EXPECT_EQ(abel::Uniform(gen, 1, 10000), 40);
        EXPECT_EQ(abel::Uniform(gen, 1, 10000), 60);
    }

    TEST(BasicMocking, DefaultArgument) {
        abel::MockingBitGen gen;

        ON_CALL(abel::MockExponential<double>(), Call(gen, 1.0))
        .WillByDefault(Return(200));

        EXPECT_EQ(abel::Exponential<double>(gen), 200);
        EXPECT_EQ(abel::Exponential<double>(gen, 1.0), 200);
    }

    TEST(BasicMocking, MultipleGenerators) {
        auto get_value = [](abel::BitGenRef gen_ref) {
            return abel::Uniform(gen_ref, 1, 1000000);
        };
        abel::MockingBitGen unmocked_generator;
        abel::MockingBitGen mocked_with_3;
        abel::MockingBitGen mocked_with_11;

        EXPECT_CALL(abel::MockUniform<int>(), Call(mocked_with_3, 1, 1000000))
                .WillOnce(Return(3))
                .WillRepeatedly(Return(17));
        EXPECT_CALL(abel::MockUniform<int>(), Call(mocked_with_11, 1, 1000000))
                .WillOnce(Return(11))
                .WillRepeatedly(Return(17));

        // Ensure that unmocked generator generates neither value.
        int unmocked_value = get_value(unmocked_generator);
        EXPECT_NE(unmocked_value, 3);
        EXPECT_NE(unmocked_value, 11);
        // Mocked generators should generate their mocked values.
        EXPECT_EQ(get_value(mocked_with_3), 3);
        EXPECT_EQ(get_value(mocked_with_11), 11);
        // Ensure that the mocks have expired.
        EXPECT_NE(get_value(mocked_with_3), 3);
        EXPECT_NE(get_value(mocked_with_11), 11);
    }

    TEST(BasicMocking, MocksNotTrigeredForIncorrectTypes) {
        abel::MockingBitGen gen;
        EXPECT_CALL(abel::MockUniform<uint32_t>(), Call(gen)).WillOnce(Return(42));

        EXPECT_NE(abel::Uniform<uint16_t>(gen), 42);  // Not mocked
        EXPECT_EQ(abel::Uniform<uint32_t>(gen), 42);  // Mock triggered
    }

    TEST(BasicMocking, FailsOnUnsatisfiedMocks) {
        EXPECT_NONFATAL_FAILURE(
                []() {
                    abel::MockingBitGen gen;
                    EXPECT_CALL(abel::MockExponential<double>(), Call(gen, 1.0))
                            .WillOnce(Return(3.0));
                    // Does not call abel::Exponential().
                }(),
                "unsatisfied and active");
    }

    TEST(OnUniform, RespectsUniformIntervalSemantics) {
        abel::MockingBitGen gen;

        EXPECT_CALL(abel::MockUniform<int>(),
                    Call(abel::IntervalClosed, gen, 1, 1000000))
        .WillOnce(Return(301));
        EXPECT_NE(abel::Uniform(gen, 1, 1000000), 301);  // Not mocked
        EXPECT_EQ(abel::Uniform(abel::IntervalClosed, gen, 1, 1000000), 301);
    }

    TEST(OnUniform, RespectsNoArgUnsignedShorthand) {
        abel::MockingBitGen gen;
        EXPECT_CALL(abel::MockUniform<uint32_t>(), Call(gen)).WillOnce(Return(42));
        EXPECT_EQ(abel::Uniform<uint32_t>(gen), 42);
    }

    TEST(RepeatedlyModifier, ForceSnakeEyesForManyDice) {
        auto roll_some_dice = [](abel::BitGenRef gen_ref) {
            std::vector<int> results(16);
            for (auto &r : results) {
                r = abel::Uniform(abel::IntervalClosed, gen_ref, 1, 6);
            }
            return results;
        };
        std::vector<int> results;
        abel::MockingBitGen gen;

        // Without any mocked calls, not all dice roll a "6".
        results = roll_some_dice(gen);
        EXPECT_LT(std::accumulate(std::begin(results), std::end(results), 0),
                  results.size() * 6);

        // Verify that we can force all "6"-rolls, with mocking.
        ON_CALL(abel::MockUniform<int>(), Call(abel::IntervalClosed, gen, 1, 6))
        .WillByDefault(Return(6));
        results = roll_some_dice(gen);
        EXPECT_EQ(std::accumulate(std::begin(results), std::end(results), 0),
                  results.size() * 6);
    }

    TEST(WillOnce, DistinctCounters) {
        abel::MockingBitGen gen;
        EXPECT_CALL(abel::MockUniform<int>(), Call(gen, 1, 1000000))
                .Times(3)
                .WillRepeatedly(Return(0));
        EXPECT_CALL(abel::MockUniform<int>(), Call(gen, 1000001, 2000000))
                .Times(3)
                .WillRepeatedly(Return(1));
        EXPECT_EQ(abel::Uniform(gen, 1000001, 2000000), 1);
        EXPECT_EQ(abel::Uniform(gen, 1, 1000000), 0);
        EXPECT_EQ(abel::Uniform(gen, 1000001, 2000000), 1);
        EXPECT_EQ(abel::Uniform(gen, 1, 1000000), 0);
        EXPECT_EQ(abel::Uniform(gen, 1000001, 2000000), 1);
        EXPECT_EQ(abel::Uniform(gen, 1, 1000000), 0);
    }

    TEST(TimesModifier, ModifierSaturatesAndExpires) {
        EXPECT_NONFATAL_FAILURE(
                []() {
                    abel::MockingBitGen gen;
                    EXPECT_CALL(abel::MockUniform<int>(), Call(gen, 1, 1000000))
                            .Times(3)
                            .WillRepeatedly(Return(15))
                            .RetiresOnSaturation();

                    EXPECT_EQ(abel::Uniform(gen, 1, 1000000), 15);
                    EXPECT_EQ(abel::Uniform(gen, 1, 1000000), 15);
                    EXPECT_EQ(abel::Uniform(gen, 1, 1000000), 15);
                    // Times(3) has expired - Should get a different value now.

                    EXPECT_NE(abel::Uniform(gen, 1, 1000000), 15);
                }(),
                "");
    }

    TEST(TimesModifier, Times0) {
        abel::MockingBitGen gen;
        EXPECT_CALL(abel::MockBernoulli(), Call(gen, 0.0)).Times(0);
        EXPECT_CALL(abel::MockPoisson<int>(), Call(gen, 1.0)).Times(0);
    }

    TEST(AnythingMatcher, MatchesAnyArgument) {
        using testing::_;

        {
            abel::MockingBitGen gen;
            ON_CALL(abel::MockUniform<int>(), Call(abel::IntervalClosed, gen, _, 1000))
            .WillByDefault(Return(11));
            ON_CALL(abel::MockUniform<int>(),
                    Call(abel::IntervalClosed, gen, _, Ne(1000)))
            .WillByDefault(Return(99));

            EXPECT_EQ(abel::Uniform(abel::IntervalClosed, gen, 10, 1000000), 99);
            EXPECT_EQ(abel::Uniform(abel::IntervalClosed, gen, 10, 1000), 11);
        }

        {
            abel::MockingBitGen gen;
            ON_CALL(abel::MockUniform<int>(), Call(gen, 1, _))
            .WillByDefault(Return(25));
            ON_CALL(abel::MockUniform<int>(), Call(gen, Ne(1), _))
            .WillByDefault(Return(99));
            EXPECT_EQ(abel::Uniform(gen, 3, 1000000), 99);
            EXPECT_EQ(abel::Uniform(gen, 1, 1000000), 25);
        }

        {
            abel::MockingBitGen gen;
            ON_CALL(abel::MockUniform<int>(), Call(gen, _, _))
            .WillByDefault(Return(145));
            EXPECT_EQ(abel::Uniform(gen, 1, 1000), 145);
            EXPECT_EQ(abel::Uniform(gen, 10, 1000), 145);
            EXPECT_EQ(abel::Uniform(gen, 100, 1000), 145);
        }
    }

    TEST(AnythingMatcher, WithWillByDefault) {
        using testing::_;
        abel::MockingBitGen gen;
        std::vector<int> values = {11, 22, 33, 44, 55, 66, 77, 88, 99, 1010};

        ON_CALL(abel::MockUniform<size_t>(), Call(gen, 0, _))
        .WillByDefault(Return(0));
        for (int i = 0; i < 100; i++) {
            auto &elem = values[abel::Uniform(gen, 0u, values.size())];
            EXPECT_EQ(elem, 11);
        }
    }

    TEST(BasicMocking, WillByDefaultWithArgs) {
        using testing::_;

        abel::MockingBitGen gen;
        ON_CALL(abel::MockPoisson<int>(), Call(gen, _))
        .WillByDefault(
                [](double lambda) { return static_cast<int>(lambda * 10); });
        EXPECT_EQ(abel::Poisson<int>(gen, 1.7), 17);
        EXPECT_EQ(abel::Poisson<int>(gen, 0.03), 0);
    }

    TEST(MockingBitGen, InSequenceSucceedsInOrder) {
        abel::MockingBitGen gen;

        testing::InSequence seq;

        EXPECT_CALL(abel::MockPoisson<int>(), Call(gen, 1.0)).WillOnce(Return(3));
        EXPECT_CALL(abel::MockPoisson<int>(), Call(gen, 2.0)).WillOnce(Return(4));

        EXPECT_EQ(abel::Poisson<int>(gen, 1.0), 3);
        EXPECT_EQ(abel::Poisson<int>(gen, 2.0), 4);
    }

}  // namespace
