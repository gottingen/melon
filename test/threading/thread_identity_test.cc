//

#include <abel/threading/internal/thread_identity.h>

#include <thread>  // NOLINT(build/c++11)
#include <vector>

#include <gtest/gtest.h>
#include <abel/base/profile.h>
#include <abel/threading/internal/spinlock.h>
#include <abel/base/profile.h>
#include <abel/synchronization/internal/per_thread_sem.h>
#include <abel/synchronization/mutex.h>

namespace abel {

    namespace threading_internal {
        namespace {

// protects num_identities_reused
            static abel::threading_internal::SpinLock map_lock(
                    abel::base_internal::kLinkerInitialized);
            static int num_identities_reused;

            static const void *const kCheckNoIdentity = reinterpret_cast<void *>(1);

            static void TestThreadIdentityCurrent(const void *assert_no_identity) {
                ThreadIdentity *identity;

                // We have to test this conditionally, because if the test framework relies
                // on abel, then some previous action may have already allocated an
                // identity.
                if (assert_no_identity == kCheckNoIdentity) {
                    identity = CurrentThreadIdentityIfPresent();
                    EXPECT_TRUE(identity == nullptr);
                }

                identity = synchronization_internal::GetOrCreateCurrentThreadIdentity();
                EXPECT_TRUE(identity != nullptr);
                ThreadIdentity *identity_no_init;
                identity_no_init = CurrentThreadIdentityIfPresent();
                EXPECT_TRUE(identity == identity_no_init);

                // Check that per_thread_synch is correctly aligned.
                EXPECT_EQ(0, reinterpret_cast<intptr_t>(&identity->per_thread_synch) %
                             PerThreadSynch::kAlignment);
                EXPECT_EQ(identity, identity->per_thread_synch.thread_identity());

                abel::threading_internal::SpinLockHolder l(&map_lock);
                num_identities_reused++;
            }

            TEST(ThreadIdentityTest, BasicIdentityWorks) {
                // This tests for the main() thread.
                TestThreadIdentityCurrent(nullptr);
            }

            TEST(ThreadIdentityTest, BasicIdentityWorksThreaded) {
                // Now try the same basic test with multiple threads being created and
                // destroyed.  This makes sure that:
                // - New threads are created without a ThreadIdentity.
                // - We re-allocate ThreadIdentity objects from the free-list.
                // - If a thread implementation chooses to recycle threads, that
                //   correct re-initialization occurs.
                static const int kNumLoops = 3;
                static const int kNumThreads = 400;
                for (int iter = 0; iter < kNumLoops; iter++) {
                    std::vector<std::thread> threads;
                    for (int i = 0; i < kNumThreads; ++i) {
                        threads.push_back(
                                std::thread(TestThreadIdentityCurrent, kCheckNoIdentity));
                    }
                    for (auto &thread : threads) {
                        thread.join();
                    }
                }

                // We should have recycled ThreadIdentity objects above; while (external)
                // library threads allocating their own identities may preclude some
                // reuse, we should have sufficient repetitions to exclude this.
                EXPECT_LT(kNumThreads, num_identities_reused);
            }

            TEST(ThreadIdentityTest, ReusedThreadIdentityMutexTest) {
                // This test repeatly creates and joins a series of threads, each of
                // which acquires and releases shared mutex locks. This verifies
                // mutex operations work correctly under a reused
                // ThreadIdentity. Note that the most likely failure mode of this
                // test is a crash or deadlock.
                static const int kNumLoops = 10;
                static const int kNumThreads = 12;
                static const int kNumMutexes = 3;
                static const int kNumLockLoops = 5;

                mutex mutexes[kNumMutexes];
                for (int iter = 0; iter < kNumLoops; ++iter) {
                    std::vector<std::thread> threads;
                    for (int thread = 0; thread < kNumThreads; ++thread) {
                        threads.push_back(std::thread([&]() {
                            for (int l = 0; l < kNumLockLoops; ++l) {
                                for (int m = 0; m < kNumMutexes; ++m) {
                                    mutex_lock lock(&mutexes[m]);
                                }
                            }
                        }));
                    }
                    for (auto &thread : threads) {
                        thread.join();
                    }
                }
            }

        }  // namespace
    }  // namespace base_internal

}  // namespace abel
