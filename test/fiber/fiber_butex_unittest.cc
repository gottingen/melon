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


#include <gtest/gtest.h>
#include <melon/utility/atomicops.h>
#include <melon/utility/time.h>
#include <melon/utility/macros.h>
#include <turbo/log/logging.h>
#include <melon/fiber/butex.h>
#include <melon/fiber/task_control.h>
#include <melon/fiber/task_group.h>
#include <melon/fiber/fiber.h>
#include <melon/fiber/unstable.h>
#include <melon/fiber/interrupt_pthread.h>

namespace fiber {
extern mutil::atomic<TaskControl*> g_task_control;
inline TaskControl* get_task_control() {
    return g_task_control.load(mutil::memory_order_consume);
}
} // namespace fiber

namespace {
TEST(ButexTest, wait_on_already_timedout_butex) {
    uint32_t* butex = fiber::butex_create_checked<uint32_t>();
    ASSERT_TRUE(butex);
    timespec now;
    ASSERT_EQ(0, clock_gettime(CLOCK_REALTIME, &now));
    *butex = 1;
    ASSERT_EQ(-1, fiber::butex_wait(butex, 1, &now));
    ASSERT_EQ(ETIMEDOUT, errno);
}

void* sleeper(void* arg) {
    fiber_usleep((uint64_t)arg);
    return NULL;
}

void* joiner(void* arg) {
    const long t1 = mutil::gettimeofday_us();
    for (fiber_t* th = (fiber_t*)arg; *th; ++th) {
        if (0 != fiber_join(*th, NULL)) {
            LOG(FATAL) << "fail to join thread_" << th - (fiber_t*)arg;
        }
        long elp = mutil::gettimeofday_us() - t1;
        EXPECT_LE(labs(elp - (th - (fiber_t*)arg + 1) * 100000L), 15000L)
            << "timeout when joining thread_" << th - (fiber_t*)arg;
        LOG(INFO) << "Joined thread " << *th << " at " << elp << "us ["
                  << fiber_self() << "]";
    }
    for (fiber_t* th = (fiber_t*)arg; *th; ++th) {
        EXPECT_EQ(0, fiber_join(*th, NULL));
    }
    return NULL;
}

struct A {
    uint64_t a;
    char dummy[0];
};

struct B {
    uint64_t a;
};


TEST(ButexTest, with_or_without_array_zero) {
    ASSERT_EQ(sizeof(B), sizeof(A));
}


TEST(ButexTest, join) {
    const size_t N = 6;
    const size_t M = 6;
    fiber_t th[N+1];
    fiber_t jth[M];
    pthread_t pth[M];
    for (size_t i = 0; i < N; ++i) {
        fiber_attr_t attr = (i == 0 ? FIBER_ATTR_PTHREAD : FIBER_ATTR_NORMAL);
        ASSERT_EQ(0, fiber_start_urgent(
                      &th[i], &attr, sleeper,
                      (void*)(100000L/*100ms*/ * (i + 1))));
    }
    th[N] = 0;  // joiner will join tids in `th' until seeing 0.
    for (size_t i = 0; i < M; ++i) {
        ASSERT_EQ(0, fiber_start_urgent(&jth[i], NULL, joiner, th));
    }
    for (size_t i = 0; i < M; ++i) {
        ASSERT_EQ(0, pthread_create(&pth[i], NULL, joiner, th));
    }
    
    for (size_t i = 0; i < M; ++i) {
        ASSERT_EQ(0, fiber_join(jth[i], NULL))
            << "i=" << i << " error=" << berror();
    }
    for (size_t i = 0; i < M; ++i) {
        ASSERT_EQ(0, pthread_join(pth[i], NULL));
    }
}


struct WaiterArg {
    int expected_result;
    int expected_value;
    mutil::atomic<int> *butex;
    const timespec *ptimeout;
};

void* waiter(void* arg) {
    WaiterArg * wa = (WaiterArg*)arg;
    const long t1 = mutil::gettimeofday_us();
    const int rc = fiber::butex_wait(
        wa->butex, wa->expected_value, wa->ptimeout);
    const long t2 = mutil::gettimeofday_us();
    if (rc == 0) {
        EXPECT_EQ(wa->expected_result, 0) << fiber_self();
    } else {
        EXPECT_EQ(wa->expected_result, errno) << fiber_self();
    }
    LOG(INFO) << "after wait, time=" << (t2-t1) << "us";
    return NULL;
}

TEST(ButexTest, sanity) {
    const size_t N = 5;
    WaiterArg args[N * 4];
    pthread_t t1, t2;
    mutil::atomic<int>* b1 =
        fiber::butex_create_checked<mutil::atomic<int> >();
    ASSERT_TRUE(b1);
    fiber::butex_destroy(b1);
    
    b1 = fiber::butex_create_checked<mutil::atomic<int> >();
    *b1 = 1;
    ASSERT_EQ(0, fiber::butex_wake(b1));

    WaiterArg *unmatched_arg = new WaiterArg;
    unmatched_arg->expected_value = *b1 + 1;
    unmatched_arg->expected_result = EWOULDBLOCK;
    unmatched_arg->butex = b1;
    unmatched_arg->ptimeout = NULL;
    pthread_create(&t2, NULL, waiter, unmatched_arg);
    fiber_t th;
    ASSERT_EQ(0, fiber_start_urgent(&th, NULL, waiter, unmatched_arg));

    const timespec abstime = mutil::seconds_from_now(1);
    for (size_t i = 0; i < 4*N; ++i) {
        args[i].expected_value = *b1;
        args[i].butex = b1;
        if ((i % 2) == 0) {
            args[i].expected_result = 0;
            args[i].ptimeout = NULL;
        } else {
            args[i].expected_result = ETIMEDOUT;
            args[i].ptimeout = &abstime;
        }
        if (i < 2*N) { 
            pthread_create(&t1, NULL, waiter, &args[i]);
        } else {
            ASSERT_EQ(0, fiber_start_urgent(&th, NULL, waiter, &args[i]));
        }
    }
    
    sleep(2);
    for (size_t i = 0; i < 2*N; ++i) {
        ASSERT_EQ(1, fiber::butex_wake(b1));
    }
    ASSERT_EQ(0, fiber::butex_wake(b1));
    sleep(1);
    fiber::butex_destroy(b1);
}


struct ButexWaitArg {
    int* butex;
    int expected_val;
    long wait_msec;
    int error_code;
};

void* wait_butex(void* void_arg) {
    ButexWaitArg* arg = static_cast<ButexWaitArg*>(void_arg);
    const timespec ts = mutil::milliseconds_from_now(arg->wait_msec);
    int rc = fiber::butex_wait(arg->butex, arg->expected_val, &ts);
    int saved_errno = errno;
    if (arg->error_code) {
        EXPECT_EQ(-1, rc);
        EXPECT_EQ(arg->error_code, saved_errno);
    } else {
        EXPECT_EQ(0, rc);
    }
    return NULL;
}

TEST(ButexTest, wait_without_stop) {
    int* butex = fiber::butex_create_checked<int>();
    *butex = 7;
    mutil::Timer tm;
    const long WAIT_MSEC = 500;
    for (int i = 0; i < 2; ++i) {
        const fiber_attr_t attr =
            (i == 0 ? FIBER_ATTR_PTHREAD : FIBER_ATTR_NORMAL);
        ButexWaitArg arg = { butex, *butex, WAIT_MSEC, ETIMEDOUT };
        fiber_t th;
        
        tm.start();
        ASSERT_EQ(0, fiber_start_urgent(&th, &attr, wait_butex, &arg));
        ASSERT_EQ(0, fiber_join(th, NULL));
        tm.stop();
        
        ASSERT_LT(labs(tm.m_elapsed() - WAIT_MSEC), 250);
    }
    fiber::butex_destroy(butex);
}

TEST(ButexTest, stop_after_running) {
    int* butex = fiber::butex_create_checked<int>();
    *butex = 7;
    mutil::Timer tm;
    const long WAIT_MSEC = 500;
    const long SLEEP_MSEC = 10;
    for (int i = 0; i < 2; ++i) {
        const fiber_attr_t attr =
            (i == 0 ? FIBER_ATTR_PTHREAD : FIBER_ATTR_NORMAL);
        fiber_t th;
        ButexWaitArg arg = { butex, *butex, WAIT_MSEC, EINTR };

        tm.start();
        ASSERT_EQ(0, fiber_start_urgent(&th, &attr, wait_butex, &arg));
        ASSERT_EQ(0, fiber_usleep(SLEEP_MSEC * 1000L));
        ASSERT_EQ(0, fiber_stop(th));
        ASSERT_EQ(0, fiber_join(th, NULL));
        tm.stop();

        ASSERT_LT(labs(tm.m_elapsed() - SLEEP_MSEC), 25);
        // ASSERT_TRUE(fiber::get_task_control()->
        //             timer_thread()._idset.empty());
        ASSERT_EQ(EINVAL, fiber_stop(th));
    }    
    fiber::butex_destroy(butex);
}

TEST(ButexTest, stop_before_running) {
    int* butex = fiber::butex_create_checked<int>();
    *butex = 7;
    mutil::Timer tm;
    const long WAIT_MSEC = 500;

    for (int i = 0; i < 2; ++i) {
        const fiber_attr_t attr =
            (i == 0 ? FIBER_ATTR_PTHREAD : FIBER_ATTR_NORMAL) | FIBER_NOSIGNAL;
        fiber_t th;
        ButexWaitArg arg = { butex, *butex, WAIT_MSEC, EINTR };
        
        tm.start();
        ASSERT_EQ(0, fiber_start_background(&th, &attr, wait_butex, &arg));
        ASSERT_EQ(0, fiber_stop(th));
        fiber_flush();
        ASSERT_EQ(0, fiber_join(th, NULL));
        tm.stop();
        
        ASSERT_LT(tm.m_elapsed(), 5);
        // ASSERT_TRUE(fiber::get_task_control()->
        //             timer_thread()._idset.empty());
        ASSERT_EQ(EINVAL, fiber_stop(th));
    }
    fiber::butex_destroy(butex);
}

void* join_the_waiter(void* arg) {
    EXPECT_EQ(0, fiber_join((fiber_t)arg, NULL));
    return NULL;
}

TEST(ButexTest, join_cant_be_wakeup) {
    const long WAIT_MSEC = 100;
    int* butex = fiber::butex_create_checked<int>();
    *butex = 7;
    mutil::Timer tm;
    ButexWaitArg arg = { butex, *butex, 1000, EINTR };

    for (int i = 0; i < 2; ++i) {
        const fiber_attr_t attr =
            (i == 0 ? FIBER_ATTR_PTHREAD : FIBER_ATTR_NORMAL);
        tm.start();
        fiber_t th, th2;
        ASSERT_EQ(0, fiber_start_urgent(&th, NULL, wait_butex, &arg));
        ASSERT_EQ(0, fiber_start_urgent(&th2, &attr, join_the_waiter, (void*)th));
        ASSERT_EQ(0, fiber_stop(th2));
        ASSERT_EQ(0, fiber_usleep(WAIT_MSEC / 2 * 1000L));
        ASSERT_TRUE(fiber::TaskGroup::exists(th));
        ASSERT_TRUE(fiber::TaskGroup::exists(th2));
        ASSERT_EQ(0, fiber_usleep(WAIT_MSEC / 2 * 1000L));
        ASSERT_EQ(0, fiber_stop(th));
        ASSERT_EQ(0, fiber_join(th2, NULL));
        ASSERT_EQ(0, fiber_join(th, NULL));
        tm.stop();
        ASSERT_LT(tm.m_elapsed(), WAIT_MSEC + 15);
        ASSERT_EQ(EINVAL, fiber_stop(th));
        ASSERT_EQ(EINVAL, fiber_stop(th2));
    }
    fiber::butex_destroy(butex);
}

TEST(ButexTest, stop_after_slept) {
    mutil::Timer tm;
    const long SLEEP_MSEC = 100;
    const long WAIT_MSEC = 10;
    
    for (int i = 0; i < 2; ++i) {
        const fiber_attr_t attr =
            (i == 0 ? FIBER_ATTR_PTHREAD : FIBER_ATTR_NORMAL);
        tm.start();
        fiber_t th;
        ASSERT_EQ(0, fiber_start_urgent(
                      &th, &attr, sleeper, (void*)(SLEEP_MSEC*1000L)));
        ASSERT_EQ(0, fiber_usleep(WAIT_MSEC * 1000L));
        ASSERT_EQ(0, fiber_stop(th));
        ASSERT_EQ(0, fiber_join(th, NULL));
        tm.stop();
        if (attr.stack_type == FIBER_STACKTYPE_PTHREAD) {
            ASSERT_LT(labs(tm.m_elapsed() - SLEEP_MSEC), 15);
        } else {
            ASSERT_LT(labs(tm.m_elapsed() - WAIT_MSEC), 15);
        }
        // ASSERT_TRUE(fiber::get_task_control()->
        //             timer_thread()._idset.empty());
        ASSERT_EQ(EINVAL, fiber_stop(th));
    }
}

TEST(ButexTest, stop_just_when_sleeping) {
    mutil::Timer tm;
    const long SLEEP_MSEC = 100;
    
    for (int i = 0; i < 2; ++i) {
        const fiber_attr_t attr =
            (i == 0 ? FIBER_ATTR_PTHREAD : FIBER_ATTR_NORMAL);
        tm.start();
        fiber_t th;
        ASSERT_EQ(0, fiber_start_urgent(
                      &th, &attr, sleeper, (void*)(SLEEP_MSEC*1000L)));
        ASSERT_EQ(0, fiber_stop(th));
        ASSERT_EQ(0, fiber_join(th, NULL));
        tm.stop();
        if (attr.stack_type == FIBER_STACKTYPE_PTHREAD) {
            ASSERT_LT(labs(tm.m_elapsed() - SLEEP_MSEC), 15);
        } else {
            ASSERT_LT(tm.m_elapsed(), 15);
        }
        // ASSERT_TRUE(fiber::get_task_control()->
        //             timer_thread()._idset.empty());
        ASSERT_EQ(EINVAL, fiber_stop(th));
    }
}

TEST(ButexTest, stop_before_sleeping) {
    mutil::Timer tm;
    const long SLEEP_MSEC = 100;

    for (int i = 0; i < 2; ++i) {
        fiber_t th;
        const fiber_attr_t attr =
            (i == 0 ? FIBER_ATTR_PTHREAD : FIBER_ATTR_NORMAL) | FIBER_NOSIGNAL;
        
        tm.start();
        ASSERT_EQ(0, fiber_start_background(&th, &attr, sleeper,
                                              (void*)(SLEEP_MSEC*1000L)));
        ASSERT_EQ(0, fiber_stop(th));
        fiber_flush();
        ASSERT_EQ(0, fiber_join(th, NULL));
        tm.stop();

        if (attr.stack_type == FIBER_STACKTYPE_PTHREAD) {
            ASSERT_LT(labs(tm.m_elapsed() - SLEEP_MSEC), 10);
        } else {
            ASSERT_LT(tm.m_elapsed(), 10);
        }
        // ASSERT_TRUE(fiber::get_task_control()->
        //             timer_thread()._idset.empty());
        ASSERT_EQ(EINVAL, fiber_stop(th));
    }
}

void* trigger_signal(void* arg) {
    pthread_t * th = (pthread_t*)arg;
    const long t1 = mutil::gettimeofday_us();
    for (size_t i = 0; i < 50; ++i) {
      usleep(100000);
      if (fiber::interrupt_pthread(*th) == ESRCH) {
        LOG(INFO) << "waiter thread end, trigger count=" << i;
        break;
      }
    }
    const long t2 = mutil::gettimeofday_us();
    LOG(INFO) << "trigger signal thread end, elapsed=" << (t2-t1) << "us";
    return NULL;
}

TEST(ButexTest, wait_with_signal_triggered) {
    mutil::Timer tm;

    const int64_t WAIT_MSEC = 500;
    WaiterArg waiter_args;
    pthread_t waiter_th, tigger_th;
    mutil::atomic<int>* butex =
        fiber::butex_create_checked<mutil::atomic<int> >();
    ASSERT_TRUE(butex);
    *butex = 1;
    ASSERT_EQ(0, fiber::butex_wake(butex));

    const timespec abstime = mutil::milliseconds_from_now(WAIT_MSEC);
    waiter_args.expected_value = *butex;
    waiter_args.butex = butex;
    waiter_args.expected_result = ETIMEDOUT;
    waiter_args.ptimeout = &abstime;
    tm.start();
    pthread_create(&waiter_th, NULL, waiter, &waiter_args);
    pthread_create(&tigger_th, NULL, trigger_signal, &waiter_th);
    
    ASSERT_EQ(0, pthread_join(waiter_th, NULL));
    tm.stop();
    auto wait_elapsed_ms = tm.m_elapsed();;
    LOG(INFO) << "waiter thread end, elapsed " << wait_elapsed_ms << " ms";

    ASSERT_LT(labs(wait_elapsed_ms - WAIT_MSEC), 250);

    ASSERT_EQ(0, pthread_join(tigger_th, NULL));
    fiber::butex_destroy(butex);
}

} // namespace
