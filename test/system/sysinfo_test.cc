// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com

#include "abel/system/sysinfo.h"

#ifndef _WIN32

#include <sys/types.h>
#include <unistd.h>

#endif

#include <thread>  // NOLINT(build/c++11)
#include <unordered_set>
#include <vector>

#include "gtest/gtest.h"
#include "abel/thread/latch.h"

namespace abel {

    namespace base_internal {
        namespace {

            TEST(SysinfoTest, num_cpus) {
                EXPECT_NE(abel::num_cpus(), 0)
                                    << "num_cpus() should not have the default value of 0";
            }

            TEST(SysinfoTest, nominal_cpu_frequency) {
#if !(defined(__aarch64__) && defined(__linux__)) && !defined(__EMSCRIPTEN__)
                EXPECT_GE(nominal_cpu_frequency(), 1000.0)
                                    << "nominal_cpu_frequency() did not return a reasonable value";
#else
                // Aarch64 cannot read the CPU frequency from sysfs, so we get back 1.0.
                // Emscripten does not have a sysfs to read from at all.
                EXPECT_EQ(nominal_cpu_frequency(), 1.0)
                    << "CPU frequency detection was fixed! Please update unittest.";
#endif
            }

            TEST(SysinfoTest, get_tid) {
                EXPECT_EQ(get_tid(), get_tid());  // Basic compile and equality test.
#ifdef __native_client__
                // Native Client has a race condition bug that leads to memory
                // exaustion when repeatedly creating and joining threads.
                // https://bugs.chromium.org/p/nativeclient/issues/detail?id=1027
                return;
#endif
                // Test that TIDs are unique to each thread.
                // Uses a few loops to exercise implementations that reallocate IDs.
                for (int i = 0; i < 32; ++i) {
                    constexpr int kNumThreads = 64;
                    abel::latch all_threads_done(kNumThreads);
                    std::vector<std::thread> threads;

                    std::mutex mutex;
                    std::unordered_set<pid_t> tids;

                    for (int j = 0; j < kNumThreads; ++j) {
                        threads.push_back(std::thread([&]() {
                            pid_t id = get_tid();
                            {
                                std::unique_lock lock(mutex);
                                ASSERT_TRUE(tids.find(id) == tids.end());
                                tids.insert(id);
                            }
                            // We can't simply join the threads here. The threads need to
                            // be alive otherwise the TID might have been reallocated to
                            // another live thread.
                            all_threads_done.arrive_and_wait();
                        }));
                    }
                    for (auto &thread : threads) {
                        thread.join();
                    }
                }
            }

#ifdef __linux__
            TEST(SysinfoTest, LinuxGetTID) {
              // On Linux, for the main thread, get_tid()==getpid() is guaranteed by the API.
              EXPECT_EQ(get_tid(), getpid());
            }
#endif

        }  // namespace
    }  // namespace base_internal

}  // namespace abel
