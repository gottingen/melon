// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com

#include <thread>  // NOLINT(build/c++11)
#include <vector>
#include "abel/thread/internal/thread_identity.h"
#include "gtest/gtest.h"
#include "abel/base/profile.h"
#include "abel/thread/spin_lock.h"
#include "abel/base/profile.h"
#include "abel/thread/internal/per_thread_sem.h"
#include "abel/thread/mutex.h"

namespace abel {

namespace thread_internal {
namespace {

// protects num_identities_reused
static abel::spin_lock map_lock(
        abel::base_internal::kLinkerInitialized);
static int num_identities_reused;

static const void *const kCheckNoIdentity = reinterpret_cast<void *>(1);

static void TestThreadIdentityCurrent(const void *assert_no_identity) {
    thread_identity *identity;

    // We have to test this conditionally, because if the test framework relies
    // on abel, then some previous action may have already allocated an
    // identity.
    if (assert_no_identity == kCheckNoIdentity) {
        identity = current_thread_identity_if_present();
        EXPECT_TRUE(identity == nullptr);
    }

    identity = thread_internal::get_or_create_current_thread_identity();
    EXPECT_TRUE(identity != nullptr);
    thread_identity *identity_no_init;
    identity_no_init = current_thread_identity_if_present();
    EXPECT_TRUE(identity == identity_no_init);

    // Check that per_thread_synch is correctly aligned.
    EXPECT_EQ(0, reinterpret_cast<intptr_t>(&identity->per_thread_synch) %
                 per_thread_synch::kAlignment);
    EXPECT_EQ(identity, identity->per_thread_synch.thread_identity());

    abel::spin_lock_holder l(&map_lock);
    num_identities_reused++;
}

TEST(ThreadIdentityTest, BasicIdentityWorks) {
    // This tests for the main() thread.
    TestThreadIdentityCurrent(nullptr);
}

TEST(ThreadIdentityTest, BasicIdentityWorksThreaded) {
    // Now try the same basic test with multiple threads being created and
    // destroyed.  This makes sure that:
    // - New threads are created without a thread_identity.
    // - We re-allocate thread_identity objects from the free-list.
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

    // We should have recycled thread_identity objects above; while (external)
    // library threads allocating their own identities may preclude some
    // reuse, we should have sufficient repetitions to exclude this.
    EXPECT_LT(kNumThreads, num_identities_reused);
}

TEST(ThreadIdentityTest, ReusedThreadIdentityMutexTest) {
    // This test repeatly creates and joins a series of threads, each of
    // which acquires and releases shared mutex locks. This verifies
    // mutex operations work correctly under a reused
    // thread_identity. Note that the most likely failure mode of this
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
