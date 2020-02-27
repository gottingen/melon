//

#include <abel/statistics/periodic_sampler.h>
#include <thread>  // NOLINT(build/c++11)
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <abel/base/profile.h>

namespace abel {
    namespace {

        using testing::Eq;
        using testing::Return;
        using testing::StrictMock;

        class mock_periodic_sampler : public periodic_sampler_base {
        public:
            virtual ~mock_periodic_sampler() = default;

            MOCK_METHOD(int, period, (), (const, noexcept));
            MOCK_METHOD(int64_t, get_exponential_biased, (int), (noexcept));
        };

        TEST(PeriodicSamplerBaseTest, Sample) {
            StrictMock<mock_periodic_sampler> sampler;

            EXPECT_CALL(sampler, period()).Times(3).WillRepeatedly(Return(16));
            EXPECT_CALL(sampler, get_exponential_biased(16))
                    .WillOnce(Return(2))
                    .WillOnce(Return(3))
                    .WillOnce(Return(4));

            EXPECT_FALSE(sampler.sample());
            EXPECT_TRUE(sampler.sample());

            EXPECT_FALSE(sampler.sample());
            EXPECT_FALSE(sampler.sample());
            EXPECT_TRUE(sampler.sample());

            EXPECT_FALSE(sampler.sample());
            EXPECT_FALSE(sampler.sample());
            EXPECT_FALSE(sampler.sample());
        }

        TEST(PeriodicSamplerBaseTest, ImmediatelySample) {
            StrictMock<mock_periodic_sampler> sampler;

            EXPECT_CALL(sampler, period()).Times(2).WillRepeatedly(Return(16));
            EXPECT_CALL(sampler, get_exponential_biased(16))
                    .WillOnce(Return(1))
                    .WillOnce(Return(2))
                    .WillOnce(Return(3));

            EXPECT_TRUE(sampler.sample());

            EXPECT_FALSE(sampler.sample());
            EXPECT_TRUE(sampler.sample());

            EXPECT_FALSE(sampler.sample());
            EXPECT_FALSE(sampler.sample());
        }

        TEST(PeriodicSamplerBaseTest, Disabled) {
            StrictMock<mock_periodic_sampler> sampler;

            EXPECT_CALL(sampler, period()).Times(3).WillRepeatedly(Return(0));

            EXPECT_FALSE(sampler.sample());
            EXPECT_FALSE(sampler.sample());
            EXPECT_FALSE(sampler.sample());
        }

        TEST(PeriodicSamplerBaseTest, AlwaysOn) {
            StrictMock<mock_periodic_sampler> sampler;

            EXPECT_CALL(sampler, period()).Times(3).WillRepeatedly(Return(1));

            EXPECT_TRUE(sampler.sample());
            EXPECT_TRUE(sampler.sample());
            EXPECT_TRUE(sampler.sample());
        }

        TEST(PeriodicSamplerBaseTest, Disable) {
            StrictMock<mock_periodic_sampler> sampler;

            EXPECT_CALL(sampler, period()).WillOnce(Return(16));
            EXPECT_CALL(sampler, get_exponential_biased(16)).WillOnce(Return(3));
            EXPECT_FALSE(sampler.sample());
            EXPECT_FALSE(sampler.sample());

            EXPECT_CALL(sampler, period()).Times(2).WillRepeatedly(Return(0));

            EXPECT_FALSE(sampler.sample());
            EXPECT_FALSE(sampler.sample());
        }

        TEST(PeriodicSamplerBaseTest, Enable) {
            StrictMock<mock_periodic_sampler> sampler;

            EXPECT_CALL(sampler, period()).WillOnce(Return(0));
            EXPECT_FALSE(sampler.sample());

            EXPECT_CALL(sampler, period()).Times(2).WillRepeatedly(Return(16));
            EXPECT_CALL(sampler, get_exponential_biased(16))
                    .Times(2)
                    .WillRepeatedly(Return(3));

            EXPECT_FALSE(sampler.sample());
            EXPECT_FALSE(sampler.sample());
            EXPECT_TRUE(sampler.sample());

            EXPECT_FALSE(sampler.sample());
            EXPECT_FALSE(sampler.sample());
        }

        TEST(PeriodicSamplerTest, ConstructConstInit) {
            struct Tag {
            };
            ABEL_CONST_INIT static periodic_sampler<Tag> sampler;
            (void) sampler;
        }

        TEST(PeriodicSamplerTest, DefaultPeriod0) {
            struct Tag {
            };
            periodic_sampler<Tag> sampler;
            EXPECT_THAT(sampler.period(), Eq(0));
        }

        TEST(PeriodicSamplerTest, DefaultPeriod) {
            struct Tag {
            };
            periodic_sampler<Tag, 100> sampler;
            EXPECT_THAT(sampler.period(), Eq(100));
        }

        TEST(PeriodicSamplerTest, SetGlobalPeriod) {
            struct Tag1 {
            };
            struct Tag2 {
            };
            periodic_sampler<Tag1, 25> sampler1;
            periodic_sampler<Tag2, 50> sampler2;

            EXPECT_THAT(sampler1.period(), Eq(25));
            EXPECT_THAT(sampler2.period(), Eq(50));

            std::thread thread([] {
                periodic_sampler<Tag1, 25> sampler1;
                periodic_sampler<Tag2, 50> sampler2;
                EXPECT_THAT(sampler1.period(), Eq(25));
                EXPECT_THAT(sampler2.period(), Eq(50));
                sampler1.SetGlobalPeriod(10);
                sampler2.SetGlobalPeriod(20);
            });
            thread.join();

            EXPECT_THAT(sampler1.period(), Eq(10));
            EXPECT_THAT(sampler2.period(), Eq(20));
        }

    }  // namespace

}  // namespace abel
