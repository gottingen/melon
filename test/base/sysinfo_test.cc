//

#include <abel/base/internal/sysinfo.h>

#ifndef _WIN32
#include <sys/types.h>
#include <unistd.h>
#endif

#include <thread>  // NOLINT(build/c++11)
#include <unordered_set>
#include <vector>

#include <gtest/gtest.h>
#include <abel/synchronization/barrier.h>
#include <abel/synchronization/mutex.h>

namespace abel {
ABEL_NAMESPACE_BEGIN
namespace base_internal {
namespace {

TEST(SysinfoTest, NumCPUs) {
  EXPECT_NE(NumCPUs(), 0)
      << "NumCPUs() should not have the default value of 0";
}

TEST(SysinfoTest, NominalCPUFrequency) {
#if !(defined(__aarch64__) && defined(__linux__)) && !defined(__EMSCRIPTEN__)
  EXPECT_GE(NominalCPUFrequency(), 1000.0)
      << "NominalCPUFrequency() did not return a reasonable value";
#else
  // Aarch64 cannot read the CPU frequency from sysfs, so we get back 1.0.
  // Emscripten does not have a sysfs to read from at all.
  EXPECT_EQ(NominalCPUFrequency(), 1.0)
      << "CPU frequency detection was fixed! Please update unittest.";
#endif
}

TEST(SysinfoTest, GetTID) {
  EXPECT_EQ(GetTID(), GetTID());  // Basic compile and equality test.
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
    barrier all_threads_done(kNumThreads);
    std::vector<std::thread> threads;

    mutex mutex;
    std::unordered_set<pid_t> tids;

    for (int j = 0; j < kNumThreads; ++j) {
      threads.push_back(std::thread([&]() {
        pid_t id = GetTID();
        {
          mutex_lock lock(&mutex);
          ASSERT_TRUE(tids.find(id) == tids.end());
          tids.insert(id);
        }
        // We can't simply join the threads here. The threads need to
        // be alive otherwise the TID might have been reallocated to
        // another live thread.
        all_threads_done.block();
      }));
    }
    for (auto& thread : threads) {
      thread.join();
    }
  }
}

#ifdef __linux__
TEST(SysinfoTest, LinuxGetTID) {
  // On Linux, for the main thread, GetTID()==getpid() is guaranteed by the API.
  EXPECT_EQ(GetTID(), getpid());
}
#endif

}  // namespace
}  // namespace base_internal
ABEL_NAMESPACE_END
}  // namespace abel
