

#include "abel/fiber/internal/scheduling_parameters.h"

#include <chrono>
#include <thread>
#include <utility>

#include "gtest/gtest.h"

#include "abel/log/logging.h"


namespace abel {
    namespace fiber_internal {

        struct Input {
            std::size_t numa_domains, available_processors, desired_concurrency;
        };

        TEST(SchedulingParameters, ComputeHeavy) {
            std::pair<Input, SchedulingParameters> kExpected[] = {
                    // (NUMA nodes, nproc, concurrency), (group size, groups, NUMA aware))
                    {{1, 45, 45}, {1, 45, false}},
                    {{1, 90, 90}, {2, 45, false}},
                    {{1, 45, 90}, {2, 45, false}},
                    {{1, 90, 45}, {1, 45, false}},
                    {{2, 40, 80}, {2, 40, false}},
                    {{2, 80, 80}, {2, 40, false}},
                    {{2, 80, 40}, {1, 40, false}},
                    {{2, 40, 40}, {1, 40, false}},
            };

            for (auto&&[input, output] : kExpected) {
                DLOG_INFO("Testing ({}, {}, {})", input.numa_domains,
                               input.available_processors, input.desired_concurrency);

                auto result = GetSchedulingParameters(
                        SchedulingProfile::ComputeHeavy, input.numa_domains,
                        input.available_processors, input.desired_concurrency);
                EXPECT_EQ(output.workers_per_group, result.workers_per_group);
                EXPECT_EQ(output.scheduling_groups, result.scheduling_groups);
                EXPECT_EQ(output.enable_numa_affinity, result.enable_numa_affinity);
            }
        }

        TEST(SchedulingParameters, Compute) {
            std::pair<Input, SchedulingParameters> kExpected[] = {
                    // (NUMA nodes, nproc, concurrency), (group size, groups, NUMA aware))
                    {{1, 45, 45}, {1, 45, false}},
                    {{1, 90, 90}, {2, 45, false}},
                    {{1, 45, 90}, {2, 45, false}},
                    {{1, 90, 45}, {1, 45, false}},
                    {{2, 40, 80}, {2, 40, true}},
                    {{2, 80, 80}, {2, 40, true}},
                    {{2, 80, 40}, {2, 20, true}},
                    {{2, 40, 40}, {2, 20, true}},
            };

            for (auto&&[input, output] : kExpected) {
                DLOG_INFO("Testing ({}, {}, {})", input.numa_domains,
                               input.available_processors, input.desired_concurrency);

                auto result = GetSchedulingParameters(
                        SchedulingProfile::Compute, input.numa_domains,
                        input.available_processors, input.desired_concurrency);
                EXPECT_EQ(output.workers_per_group, result.workers_per_group);
                EXPECT_EQ(output.scheduling_groups, result.scheduling_groups);
                EXPECT_EQ(output.enable_numa_affinity, result.enable_numa_affinity);
            }
        }

        TEST(SchedulingParameters, Neutral) {
            std::pair<Input, SchedulingParameters> kExpected[] = {
                    // (NUMA nodes, nproc, concurrency), (group size, groups, NUMA aware))
                    {{1, 45, 45}, {2, 23, false}},
                    {{1, 90, 90}, {5, 18, false}},
                    {{1, 45, 90}, {5, 18, false}},
                    {{1, 90, 45}, {2, 23, false}},
                    {{2, 40, 80}, {4, 20, true}},
                    {{2, 80, 80}, {4, 20, true}},
                    {{2, 80, 40}, {2, 20, true}},
                    {{2, 40, 40}, {2, 20, true}},
                    {{2, 76, 32}, {2, 16, true}},
                    {{2, 76, 40}, {2, 20, true}},
            };

            for (auto&&[input, output] : kExpected) {
                DLOG_INFO("Testing ({}, {}, {})", input.numa_domains,
                               input.available_processors, input.desired_concurrency);

                auto result = GetSchedulingParameters(
                        SchedulingProfile::Neutral, input.numa_domains,
                        input.available_processors, input.desired_concurrency);
                EXPECT_EQ(output.workers_per_group, result.workers_per_group);
                EXPECT_EQ(output.scheduling_groups, result.scheduling_groups);
                EXPECT_EQ(output.enable_numa_affinity, result.enable_numa_affinity);
            }
        }

        TEST(SchedulingParameters, Io) {
            std::pair<Input, SchedulingParameters> kExpected[] = {
                    // (NUMA nodes, nproc, concurrency), (group size, groups, NUMA aware))
                    {{1, 45, 45}, {3, 15, false}},
                    {{1, 90, 90}, {6, 15, false}},
                    {{1, 45, 90}, {6, 15, false}},
                    {{1, 90, 45}, {3, 15, false}},
                    {{2, 40, 80}, {4, 20, true}},
                    {{2, 80, 80}, {4, 20, true}},
                    {{2, 80, 40}, {2, 20, true}},
                    {{2, 40, 40}, {2, 20, true}},
                    {{2, 80, 90}, {6, 15, true}},
                    {{2, 80, 45}, {2, 23, true}},
                    {{2, 80, 85}, {4, 22, true}},
                    {{2, 80, 77}, {6, 13, true}},
                    {{2, 76, 32}, {2, 16, true}},
                    {{2, 76, 40}, {2, 20, true}},
            };

            for (auto&&[input, output] : kExpected) {
                DLOG_INFO("Testing ({}, {}, {})", input.numa_domains,
                               input.available_processors, input.desired_concurrency);

                auto result = GetSchedulingParameters(
                        SchedulingProfile::Io, input.numa_domains, input.available_processors,
                        input.desired_concurrency);
                EXPECT_EQ(output.workers_per_group, result.workers_per_group);
                EXPECT_EQ(output.scheduling_groups, result.scheduling_groups);
                EXPECT_EQ(output.enable_numa_affinity, result.enable_numa_affinity);
            }
        }

        TEST(SchedulingParameters, IoHeavy) {
            std::pair<Input, SchedulingParameters> kExpected[] = {
                    // (NUMA nodes, nproc, concurrency), (group size, groups, NUMA aware))
                    {{1, 45, 45}, {5,  9,  false}},
                    {{1, 90, 90}, {10, 9,  false}},
                    {{1, 45, 90}, {10, 9,  false}},
                    {{1, 90, 45}, {5,  9,  false}},
                    {{2, 40, 80}, {10, 8,  true}},
                    {{2, 80, 80}, {10, 8,  true}},
                    {{2, 80, 40}, {4,  10, true}},
                    {{2, 40, 40}, {4,  10, true}},
                    {{2, 76, 32}, {4,  8,  true}},
                    {{2, 76, 40}, {4,  10, true}},
            };

            for (auto&&[input, output] : kExpected) {
                DLOG_INFO("Testing ({}, {}, {})", input.numa_domains,
                               input.available_processors, input.desired_concurrency);

                auto result = GetSchedulingParameters(
                        SchedulingProfile::IoHeavy, input.numa_domains,
                        input.available_processors, input.desired_concurrency);
                EXPECT_EQ(output.workers_per_group, result.workers_per_group);
                EXPECT_EQ(output.scheduling_groups, result.scheduling_groups);
                EXPECT_EQ(output.enable_numa_affinity, result.enable_numa_affinity);
            }
        }

    }  // namespace fiber_internal
}  // namespace abel

//int main(int argc, char** argv) {
//  testing::InitGoogleTest(&argc, argv);
//  FLAGS_logtostderr = true;
//  return RUN_ALL_TESTS();
//}
