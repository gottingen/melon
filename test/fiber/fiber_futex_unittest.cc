//
// Copyright (C) 2024 EA group inc.
// Author: Jeff.li lijippy@163.com
// All rights reserved.
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//
//


#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <gtest/gtest.h>
#include <melon/utility/time.h>
#include <melon/utility/macros.h>
#include <melon/base/errno.h>
#include <limits.h>                            // INT_MAX
#include <melon/utility/atomicops.h>
#include <melon/fiber/fiber.h>
#include <melon/fiber/sys_futex.h>
#include <melon/fiber/processor.h>
#include <cinttypes>

namespace {
volatile bool stop = false;

std::atomic<int> nthread(0);

void* read_thread(void* arg) {
    std::atomic<int>* m = (std::atomic<int>*)arg;
    int njob = 0;
    while (!stop) {
        int x;
        while (!stop && (x = *m) != 0) {
            if (x > 0) {
                while ((x = m->fetch_sub(1)) > 0) {
                    ++njob;
                    const long start = mutil::cpuwide_time_ns();
                    while (mutil::cpuwide_time_ns() < start + 10000) {
                    }
                    if (stop) {
                        return new int(njob);
                    }
                }
                m->fetch_add(1);
            } else {
                cpu_relax();
            }
        }

        ++nthread;
        fiber::futex_wait_private(m/*lock1*/, 0/*consumed_njob*/, NULL);
        --nthread;
    }
    return new int(njob);
}

TEST(FutexTest, rdlock_performance) {
    const size_t N = 100000;
    std::atomic<int> lock1(0);
    pthread_t rth[8];
    for (size_t i = 0; i < ARRAY_SIZE(rth); ++i) {
        ASSERT_EQ(0, pthread_create(&rth[i], NULL, read_thread, &lock1));
    }

    const int64_t t1 = mutil::cpuwide_time_ns();
    for (size_t i = 0; i < N; ++i) {
        if (nthread) {
            lock1.fetch_add(1);
            fiber::futex_wake_private(&lock1, 1);
        } else {
            lock1.fetch_add(1);
            if (nthread) {
                fiber::futex_wake_private(&lock1, 1);
            }
        }
    }
    const int64_t t2 = mutil::cpuwide_time_ns();

    fiber_usleep(3000000);
    stop = true;
    for (int i = 0; i < 10; ++i) {
        fiber::futex_wake_private(&lock1, INT_MAX);
        sched_yield();
    }

    int njob = 0;
    int* res;
    for (size_t i = 0; i < ARRAY_SIZE(rth); ++i) {
        pthread_join(rth[i], (void**)&res);
        njob += *res;
        delete res;
    }
    printf("wake %lu times, %" PRId64 "ns each, lock1=%d njob=%d\n",
           N, (t2-t1)/N, lock1.load(), njob);
    ASSERT_EQ(N, (size_t)(lock1.load() + njob));
}

TEST(FutexTest, futex_wake_before_wait) {
    int lock1 = 0;
    timespec timeout = { 1, 0 };
    ASSERT_EQ(0, fiber::futex_wake_private(&lock1, INT_MAX));
    ASSERT_EQ(-1, fiber::futex_wait_private(&lock1, 0, &timeout));
    ASSERT_EQ(ETIMEDOUT, errno);
}

void* dummy_waiter(void* lock) {
    fiber::futex_wait_private(lock, 0, NULL);
    return NULL;
}

TEST(FutexTest, futex_wake_many_waiters_perf) {
    
    int lock1 = 0;
    size_t N = 0;
    pthread_t th;
    for (; N < 1000 && !pthread_create(&th, NULL, dummy_waiter, &lock1); ++N) {}
    
    sleep(1);
    int nwakeup = 0;
    mutil::Timer tm;
    tm.start();
    for (size_t i = 0; i < N; ++i) {
        nwakeup += fiber::futex_wake_private(&lock1, 1);
    }
    tm.stop();
    printf("N=%lu, futex_wake a thread = %" PRId64 "ns\n", N, tm.n_elapsed() / N);
    ASSERT_EQ(N, (size_t)nwakeup);

    sleep(2);
    const size_t REP = 10000;
    nwakeup = 0;
    tm.start();
    for (size_t i = 0; i < REP; ++i) {
        nwakeup += fiber::futex_wake_private(&lock1, 1);
    }
    tm.stop();
    ASSERT_EQ(0, nwakeup);
    printf("futex_wake nop = %" PRId64 "ns\n", tm.n_elapsed() / REP);
}

std::atomic<int> nevent(0);

void* waker(void* lock) {
    fiber_usleep(10000);
    const size_t REP = 100000;
    int nwakeup = 0;
    mutil::Timer tm;
    tm.start();
    for (size_t i = 0; i < REP; ++i) {
        nwakeup += fiber::futex_wake_private(lock, 1);
    }
    tm.stop();
    EXPECT_EQ(0, nwakeup);
    printf("futex_wake nop = %" PRId64 "ns\n", tm.n_elapsed() / REP);
    return NULL;
} 

void* batch_waker(void* lock) {
    fiber_usleep(10000);
    const size_t REP = 100000;
    int nwakeup = 0;
    mutil::Timer tm;
    tm.start();
    for (size_t i = 0; i < REP; ++i) {
        if (nevent.fetch_add(1, std::memory_order_relaxed) == 0) {
            nwakeup += fiber::futex_wake_private(lock, 1);
            int expected = 1;
            while (1) {
                int last_expected = expected;
                if (nevent.compare_exchange_strong(expected, 0, std::memory_order_relaxed)) {
                    break;
                }
                nwakeup += fiber::futex_wake_private(lock, expected - last_expected);
            }
        }
    }
    tm.stop();
    EXPECT_EQ(0, nwakeup);
    printf("futex_wake nop = %" PRId64 "ns\n", tm.n_elapsed() / REP);
    return NULL;
} 

TEST(FutexTest, many_futex_wake_nop_perf) {
    pthread_t th[8];
    int lock1;
    std::cout << "[Direct wake]" << std::endl;
    for (size_t i = 0; i < ARRAY_SIZE(th); ++i) {
        ASSERT_EQ(0, pthread_create(&th[i], NULL, waker, &lock1));
    }
    for (size_t i = 0; i < ARRAY_SIZE(th); ++i) {
        ASSERT_EQ(0, pthread_join(th[i], NULL));
    }
    std::cout << "[Batch wake]" << std::endl;
    for (size_t i = 0; i < ARRAY_SIZE(th); ++i) {
        ASSERT_EQ(0, pthread_create(&th[i], NULL, batch_waker, &lock1));
    }
    for (size_t i = 0; i < ARRAY_SIZE(th); ++i) {
        ASSERT_EQ(0, pthread_join(th[i], NULL));
    }
}
} // namespace
