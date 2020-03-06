//

#include <testing/mock_distributions.h>

#include <gtest/gtest.h>
#include <testing/mocking_bit_gen.h>
#include <abel/stats/random/random.h>

namespace {
    using ::testing::Return;

    TEST(MockDistributions, Examples) {
        abel::MockingBitGen gen;

        EXPECT_NE(abel::uniform<int>(gen, 1, 1000000), 20);
        EXPECT_CALL(abel::MockUniform<int>(), call(gen, 1, 1000000))
        .WillOnce(Return(20));
        EXPECT_EQ(abel::uniform<int>(gen, 1, 1000000), 20);

        EXPECT_NE(abel::uniform<double>(gen, 0.0, 100.0), 5.0);
        EXPECT_CALL(abel::MockUniform<double>(), call(gen, 0.0, 100.0))
        .WillOnce(Return(5.0));
        EXPECT_EQ(abel::uniform<double>(gen, 0.0, 100.0), 5.0);

        EXPECT_NE(abel::Exponential<double>(gen, 1.0), 42);
        EXPECT_CALL(abel::MockExponential<double>(), call(gen, 1.0))
        .WillOnce(Return(42));
        EXPECT_EQ(abel::Exponential<double>(gen, 1.0), 42);

        EXPECT_NE(abel::Poisson<int>(gen, 1.0), 500);
        EXPECT_CALL(abel::MockPoisson<int>(), call(gen, 1.0)).WillOnce(Return(500));
        EXPECT_EQ(abel::Poisson<int>(gen, 1.0), 500);

        EXPECT_NE(abel::Bernoulli(gen, 0.000001), true);
        EXPECT_CALL(abel::MockBernoulli(), call(gen, 0.000001))
        .WillOnce(Return(true));
        EXPECT_EQ(abel::Bernoulli(gen, 0.000001), true);

        EXPECT_NE(abel::Beta<double>(gen, 3.0, 2.0), 0.567);
        EXPECT_CALL(abel::MockBeta<double>(), call(gen, 3.0, 2.0))
        .WillOnce(Return(0.567));
        EXPECT_EQ(abel::Beta<double>(gen, 3.0, 2.0), 0.567);

        EXPECT_NE(abel::Zipf<int>(gen, 1000000, 2.0, 1.0), 1221);
        EXPECT_CALL(abel::MockZipf<int>(), call(gen, 1000000, 2.0, 1.0))
        .WillOnce(Return(1221));
        EXPECT_EQ(abel::Zipf<int>(gen, 1000000, 2.0, 1.0), 1221);

        EXPECT_NE(abel::Gaussian<double>(gen, 0.0, 1.0), 0.001);
        EXPECT_CALL(abel::MockGaussian<double>(), call(gen, 0.0, 1.0))
        .WillOnce(Return(0.001));
        EXPECT_EQ(abel::Gaussian<double>(gen, 0.0, 1.0), 0.001);

        EXPECT_NE(abel::LogUniform<int>(gen, 0, 1000000, 2), 2040);
        EXPECT_CALL(abel::MockLogUniform<int>(), call(gen, 0, 1000000, 2))
        .WillOnce(Return(2040));
        EXPECT_EQ(abel::LogUniform<int>(gen, 0, 1000000, 2), 2040);
    }

}  // namespace
