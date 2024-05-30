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


#include <execinfo.h>
#include <gtest/gtest.h>
#include <melon/utility/time.h>
#include <melon/utility/macros.h>
#include <turbo/log/logging.h>
#include <turbo/log/logging.h>
#include <melon/utility/gperftools_profiler.h>
#include <melon/fiber/fiber.h>
#include <melon/fiber/unstable.h>
#include <melon/fiber/task_meta.h>

namespace fiber {
    extern __thread fiber::LocalStorage tls_bls;
}

namespace {
class FiberTest : public ::testing::Test{
protected:
    FiberTest(){
        const int kNumCores = sysconf(_SC_NPROCESSORS_ONLN);
        if (kNumCores > 0) {
            fiber_setconcurrency(kNumCores);
        }
    };
    virtual ~FiberTest(){};
    virtual void SetUp() {
    };
    virtual void TearDown() {
    };
};

TEST_F(FiberTest, sizeof_task_meta) {
    LOG(INFO) << "sizeof(TaskMeta)=" << sizeof(fiber::TaskMeta);
}

void* unrelated_pthread(void*) {
    LOG(INFO) << "I did not call any fiber function, "
        "I should begin and end without any problem";
    return (void*)(intptr_t)1;
}

TEST_F(FiberTest, unrelated_pthread) {
    pthread_t th;
    ASSERT_EQ(0, pthread_create(&th, NULL, unrelated_pthread, NULL));
    void* ret = NULL;
    ASSERT_EQ(0, pthread_join(th, &ret));
    ASSERT_EQ(1, (intptr_t)ret);
}

TEST_F(FiberTest, attr_init_and_destroy) {
    fiber_attr_t attr;
    ASSERT_EQ(0, fiber_attr_init(&attr));
    ASSERT_EQ(0, fiber_attr_destroy(&attr));
}

fiber_fcontext_t fcm;
fiber_fcontext_t fc;
typedef std::pair<int,int> pair_t;
static void f(intptr_t param) {
    pair_t* p = (pair_t*)param;
    p = (pair_t*)fiber_jump_fcontext(&fc, fcm, (intptr_t)(p->first+p->second));
    fiber_jump_fcontext(&fc, fcm, (intptr_t)(p->first+p->second));
}

TEST_F(FiberTest, context_sanity) {
    fcm = NULL;
    std::size_t size(8192);
    void* sp = malloc(size);

    pair_t p(std::make_pair(2, 7));
    fc = fiber_make_fcontext((char*)sp + size, size, f);

    int res = (int)fiber_jump_fcontext(&fcm, fc, (intptr_t)&p);
    std::cout << p.first << " + " << p.second << " == " << res << std::endl;

    p = std::make_pair(5, 6);
    res = (int)fiber_jump_fcontext(&fcm, fc, (intptr_t)&p);
    std::cout << p.first << " + " << p.second << " == " << res << std::endl;
}

TEST_F(FiberTest, call_fiber_functions_before_tls_created) {
    ASSERT_EQ(0, fiber_usleep(1000));
    ASSERT_EQ(EINVAL, fiber_join(0, NULL));
    ASSERT_EQ(0UL, fiber_self());
}

mutil::atomic<bool> stop(false);

void* sleep_for_awhile(void* arg) {
    LOG(INFO) << "sleep_for_awhile(" << arg << ")";
    fiber_usleep(100000L);
    LOG(INFO) << "sleep_for_awhile(" << arg << ") wakes up";
    return NULL;
}

void* just_exit(void* arg) {
    LOG(INFO) << "just_exit(" << arg << ")";
    fiber_exit(NULL);
    EXPECT_TRUE(false) << "just_exit(" << arg << ") should never be here";
    return NULL;
}

void* repeated_sleep(void* arg) {
    for (size_t i = 0; !stop; ++i) {
        LOG(INFO) << "repeated_sleep(" << arg << ") i=" << i;
        fiber_usleep(1000000L);
    }
    return NULL;
}

void* spin_and_log(void* arg) {
    // This thread never yields CPU.
    mutil::EveryManyUS every_1s(1000000L);
    size_t i = 0;
    while (!stop) {
        if (every_1s) {
            LOG(INFO) << "spin_and_log(" << arg << ")=" << i++;
        }
    }
    return NULL;
}

void* do_nothing(void* arg) {
    LOG(INFO) << "do_nothing(" << arg << ")";
    return NULL;
}

void* launcher(void* arg) {
    LOG(INFO) << "launcher(" << arg << ")";
    for (size_t i = 0; !stop; ++i) {
        fiber_t th;
        fiber_start_urgent(&th, NULL, do_nothing, (void*)i);
        fiber_usleep(1000000L);
    }
    return NULL;
}

void* stopper(void*) {
    // Need this thread to set `stop' to true. Reason: If spin_and_log (which
    // never yields CPU) is scheduled to main thread, main thread cannot get
    // to run again.
    fiber_usleep(5*1000000L);
    LOG(INFO) << "about to stop";
    stop = true;
    return NULL;
}

void* misc(void* arg) {
    LOG(INFO) << "misc(" << arg << ")";
    fiber_t th[8];
    EXPECT_EQ(0, fiber_start_urgent(&th[0], NULL, sleep_for_awhile, (void*)2));
    EXPECT_EQ(0, fiber_start_urgent(&th[1], NULL, just_exit, (void*)3));
    EXPECT_EQ(0, fiber_start_urgent(&th[2], NULL, repeated_sleep, (void*)4));
    EXPECT_EQ(0, fiber_start_urgent(&th[3], NULL, repeated_sleep, (void*)68));
    EXPECT_EQ(0, fiber_start_urgent(&th[4], NULL, spin_and_log, (void*)5));
    EXPECT_EQ(0, fiber_start_urgent(&th[5], NULL, spin_and_log, (void*)85));
    EXPECT_EQ(0, fiber_start_urgent(&th[6], NULL, launcher, (void*)6));
    EXPECT_EQ(0, fiber_start_urgent(&th[7], NULL, stopper, NULL));
    for (size_t i = 0; i < ARRAY_SIZE(th); ++i) {
        EXPECT_EQ(0, fiber_join(th[i], NULL));
    }
    return NULL;
}

TEST_F(FiberTest, sanity) {
    LOG(INFO) << "main thread " << pthread_self();
    fiber_t th1;
    ASSERT_EQ(0, fiber_start_urgent(&th1, NULL, misc, (void*)1));
    LOG(INFO) << "back to main thread " << th1 << " " << pthread_self();
    ASSERT_EQ(0, fiber_join(th1, NULL));
}

const size_t BT_SIZE = 64;
void *bt_array[BT_SIZE];
int bt_cnt;

int do_bt (void) {
    bt_cnt = backtrace (bt_array, BT_SIZE);
    return 56;
}

int call_do_bt (void) {
    return do_bt () + 1;
}

void * tf (void*) {
    if (call_do_bt () != 57) {
        return (void *) 1L;
    }
    return NULL;
}

TEST_F(FiberTest, backtrace) {
    fiber_t th;
    ASSERT_EQ(0, fiber_start_urgent(&th, NULL, tf, NULL));
    ASSERT_EQ(0, fiber_join (th, NULL));

    char **text = backtrace_symbols (bt_array, bt_cnt);
    ASSERT_TRUE(text);
    for (int i = 0; i < bt_cnt; ++i) {
        puts(text[i]);
    }
}

void* show_self(void*) {
    EXPECT_NE(0ul, fiber_self());
    LOG(INFO) << "fiber_self=" << fiber_self();
    return NULL;
}

TEST_F(FiberTest, fiber_self) {
    ASSERT_EQ(0ul, fiber_self());
    fiber_t bth;
    ASSERT_EQ(0, fiber_start_urgent(&bth, NULL, show_self, NULL));
    ASSERT_EQ(0, fiber_join(bth, NULL));
}

void* join_self(void*) {
    EXPECT_EQ(EINVAL, fiber_join(fiber_self(), NULL));
    return NULL;
}

TEST_F(FiberTest, fiber_join) {
    // Invalid tid
    ASSERT_EQ(EINVAL, fiber_join(0, NULL));
    
    // Unexisting tid
    ASSERT_EQ(EINVAL, fiber_join((fiber_t)-1, NULL));

    // Joining self
    fiber_t th;
    ASSERT_EQ(0, fiber_start_urgent(&th, NULL, join_self, NULL));
}

void* change_errno(void* arg) {
    errno = (intptr_t)arg;
    return NULL;
}

TEST_F(FiberTest, errno_not_changed) {
    fiber_t th;
    errno = 1;
    fiber_start_urgent(&th, NULL, change_errno, (void*)(intptr_t)2);
    ASSERT_EQ(1, errno);
}

static long sleep_in_adding_func = 0;

void* adding_func(void* arg) {
    mutil::atomic<size_t>* s = (mutil::atomic<size_t>*)arg;
    if (sleep_in_adding_func > 0) {
        long t1 = 0;
        if (10000 == s->fetch_add(1)) {
            t1 = mutil::cpuwide_time_us();
        }
        fiber_usleep(sleep_in_adding_func);
        if (t1) {
            LOG(INFO) << "elapse is " << mutil::cpuwide_time_us() - t1 << "ns";
        }
    } else {
        s->fetch_add(1);
    }
    return NULL;
}

TEST_F(FiberTest, small_threads) {
    for (size_t z = 0; z < 2; ++z) {
        sleep_in_adding_func = (z ? 1 : 0);
        char prof_name[32];
        if (sleep_in_adding_func) {
            snprintf(prof_name, sizeof(prof_name), "smallthread.prof");
        } else {
            snprintf(prof_name, sizeof(prof_name), "smallthread_nosleep.prof");
        }

        mutil::atomic<size_t> s(0);
        size_t N = (sleep_in_adding_func ? 40000 : 100000);
        std::vector<fiber_t> th;
        th.reserve(N);
        mutil::Timer tm;
        for (size_t j = 0; j < 3; ++j) {
            th.clear();
            if (j == 1) {
                ProfilerStart(prof_name);
            }
            tm.start();
            for (size_t i = 0; i < N; ++i) {
                fiber_t t1;
                ASSERT_EQ(0, fiber_start_urgent(
                              &t1, &FIBER_ATTR_SMALL, adding_func, &s));
                th.push_back(t1);
            }
            tm.stop();
            if (j == 1) {
                ProfilerStop();
            }
            for (size_t i = 0; i < N; ++i) {
                fiber_join(th[i], NULL);
            }
            LOG(INFO) << "[Round " << j + 1 << "] fiber_start_urgent takes "
                      << tm.n_elapsed()/N << "ns, sum=" << s;
            ASSERT_EQ(N * (j + 1), (size_t)s);
        
            // Check uniqueness of th
            std::sort(th.begin(), th.end());
            ASSERT_EQ(th.end(), std::unique(th.begin(), th.end()));
        }
    }
}

void* fiber_starter(void* void_counter) {
    while (!stop.load(mutil::memory_order_relaxed)) {
        fiber_t th;
        EXPECT_EQ(0, fiber_start_urgent(&th, NULL, adding_func, void_counter));
    }
    return NULL;
}

struct MELON_CACHELINE_ALIGNMENT AlignedCounter {
    AlignedCounter() : value(0) {}
    mutil::atomic<size_t> value;
};

TEST_F(FiberTest, start_fibers_frequently) {
    sleep_in_adding_func = 0;
    char prof_name[32];
    snprintf(prof_name, sizeof(prof_name), "start_fibers_frequently.prof");
    const int con = fiber_getconcurrency();
    ASSERT_GT(con, 0);
    AlignedCounter* counters = new AlignedCounter[con];
    fiber_t th[con];

    std::cout << "Perf with different parameters..." << std::endl;
    //ProfilerStart(prof_name);
    for (int cur_con = 1; cur_con <= con; ++cur_con) {
        stop = false;
        for (int i = 0; i < cur_con; ++i) {
            counters[i].value = 0;
            ASSERT_EQ(0, fiber_start_urgent(
                          &th[i], NULL, fiber_starter, &counters[i].value));
        }
        mutil::Timer tm;
        tm.start();
        fiber_usleep(200000L);
        stop = true;
        for (int i = 0; i < cur_con; ++i) {
            fiber_join(th[i], NULL);
        }
        tm.stop();
        size_t sum = 0;
        for (int i = 0; i < cur_con; ++i) {
            sum += counters[i].value * 1000 / tm.m_elapsed();
        }
        std::cout << sum << ",";
    }
    std::cout << std::endl;
    //ProfilerStop();
    delete [] counters;
}

void* log_start_latency(void* void_arg) {
    mutil::Timer* tm = static_cast<mutil::Timer*>(void_arg);
    tm->stop();
    return NULL;
}

TEST_F(FiberTest, start_latency_when_high_idle) {
    bool warmup = true;
    long elp1 = 0;
    long elp2 = 0;
    int REP = 0;
    for (int i = 0; i < 10000; ++i) {
        mutil::Timer tm;
        tm.start();
        fiber_t th;
        fiber_start_urgent(&th, NULL, log_start_latency, &tm);
        fiber_join(th, NULL);
        fiber_t th2;
        mutil::Timer tm2;
        tm2.start();
        fiber_start_background(&th2, NULL, log_start_latency, &tm2);
        fiber_join(th2, NULL);
        if (!warmup) {
            ++REP;
            elp1 += tm.n_elapsed();
            elp2 += tm2.n_elapsed();
        } else if (i == 100) {
            warmup = false;
        }
    }
    LOG(INFO) << "start_urgent=" << elp1 / REP << "ns start_background="
              << elp2 / REP << "ns";
}

void* sleep_for_awhile_with_sleep(void* arg) {
    fiber_usleep((intptr_t)arg);
    return NULL;
}

TEST_F(FiberTest, stop_sleep) {
    fiber_t th;
    ASSERT_EQ(0, fiber_start_urgent(
                  &th, NULL, sleep_for_awhile_with_sleep, (void*)1000000L));
    mutil::Timer tm;
    tm.start();
    fiber_usleep(10000);
    ASSERT_EQ(0, fiber_stop(th));
    ASSERT_EQ(0, fiber_join(th, NULL));
    tm.stop();
    ASSERT_LE(labs(tm.m_elapsed() - 10), 10);
}

TEST_F(FiberTest, fiber_exit) {
    fiber_t th1;
    fiber_t th2;
    pthread_t th3;
    fiber_t th4;
    fiber_t th5;
    const fiber_attr_t attr = FIBER_ATTR_PTHREAD;

    ASSERT_EQ(0, fiber_start_urgent(&th1, NULL, just_exit, NULL));
    ASSERT_EQ(0, fiber_start_background(&th2, NULL, just_exit, NULL));
    ASSERT_EQ(0, pthread_create(&th3, NULL, just_exit, NULL));
    EXPECT_EQ(0, fiber_start_urgent(&th4, &attr, just_exit, NULL));
    EXPECT_EQ(0, fiber_start_background(&th5, &attr, just_exit, NULL));

    ASSERT_EQ(0, fiber_join(th1, NULL));
    ASSERT_EQ(0, fiber_join(th2, NULL));
    ASSERT_EQ(0, pthread_join(th3, NULL));
    ASSERT_EQ(0, fiber_join(th4, NULL));
    ASSERT_EQ(0, fiber_join(th5, NULL));
}

TEST_F(FiberTest, fiber_equal) {
    fiber_t th1;
    ASSERT_EQ(0, fiber_start_urgent(&th1, NULL, do_nothing, NULL));
    fiber_t th2;
    ASSERT_EQ(0, fiber_start_urgent(&th2, NULL, do_nothing, NULL));
    ASSERT_EQ(0, fiber_equal(th1, th2));
    fiber_t th3 = th2;
    ASSERT_EQ(1, fiber_equal(th3, th2));
    ASSERT_EQ(0, fiber_join(th1, NULL));
    ASSERT_EQ(0, fiber_join(th2, NULL));
}

void* mark_run(void* run) {
    *static_cast<pthread_t*>(run) = pthread_self();
    return NULL;
}

void* check_sleep(void* pthread_task) {
    EXPECT_TRUE(fiber_self() != 0);
    // Create a no-signal task that other worker will not steal. The task will be
    // run if current fiber does context switch.
    fiber_attr_t attr = FIBER_ATTR_NORMAL | FIBER_NOSIGNAL;
    fiber_t th1;
    pthread_t run = 0;
    const pthread_t pid = pthread_self();
    EXPECT_EQ(0, fiber_start_urgent(&th1, &attr, mark_run, &run));
    if (pthread_task) {
        fiber_usleep(100000L);
        // due to NOSIGNAL, mark_run did not run.
        // FIXME: actually runs. someone is still stealing.
        // EXPECT_EQ((pthread_t)0, run);
        // fiber_usleep = usleep for FIBER_ATTR_PTHREAD
        EXPECT_EQ(pid, pthread_self());
        // schedule mark_run
        fiber_flush();
    } else {
        // start_urgent should jump to the new thread first, then back to
        // current thread.
        EXPECT_EQ(pid, run);             // should run in the same pthread
    }
    EXPECT_EQ(0, fiber_join(th1, NULL));
    if (pthread_task) {
        EXPECT_EQ(pid, pthread_self());
        EXPECT_NE((pthread_t)0, run); // the mark_run should run.
    }
    return NULL;
}

TEST_F(FiberTest, fiber_usleep) {
    // NOTE: May fail because worker threads may still be stealing tasks
    // after previous cases.
    usleep(10000);
    
    fiber_t th1;
    ASSERT_EQ(0, fiber_start_urgent(&th1, &FIBER_ATTR_PTHREAD,
                                      check_sleep, (void*)1));
    ASSERT_EQ(0, fiber_join(th1, NULL));
    
    fiber_t th2;
    ASSERT_EQ(0, fiber_start_urgent(&th2, NULL,
                                      check_sleep, (void*)0));
    ASSERT_EQ(0, fiber_join(th2, NULL));
}

static const fiber_attr_t FIBER_ATTR_NORMAL_WITH_SPAN =
{ FIBER_STACKTYPE_NORMAL, FIBER_INHERIT_SPAN, NULL };

void* test_parent_span(void* p) {
    uint64_t *q = (uint64_t *)p;
    *q = (uint64_t)(fiber::tls_bls.rpcz_parent_span);
    LOG(INFO) << "span id in thread is " << *q;
    return NULL;
}

TEST_F(FiberTest, test_span) {
    uint64_t p1 = 0;
    uint64_t p2 = 0;

    uint64_t target = 0xBADBEAFUL;
    LOG(INFO) << "target span id is " << target;

    fiber::tls_bls.rpcz_parent_span = (void*)target;
    fiber_t th1;
    ASSERT_EQ(0, fiber_start_urgent(&th1, &FIBER_ATTR_NORMAL_WITH_SPAN,
                                      test_parent_span, &p1));
    ASSERT_EQ(0, fiber_join(th1, NULL));

    fiber_t th2;
    ASSERT_EQ(0, fiber_start_background(&th2, NULL,
                                      test_parent_span, &p2));
    ASSERT_EQ(0, fiber_join(th2, NULL));

    ASSERT_EQ(p1, target);
    ASSERT_NE(p2, target);
}

void* dummy_thread(void*) {
    return NULL;
}

TEST_F(FiberTest, too_many_nosignal_threads) {
    for (size_t i = 0; i < 100000; ++i) {
        fiber_attr_t attr = FIBER_ATTR_NORMAL | FIBER_NOSIGNAL;
        fiber_t tid;
        ASSERT_EQ(0, fiber_start_urgent(&tid, &attr, dummy_thread, NULL));
    }
}

static void* yield_thread(void*) {
    fiber_yield();
    return NULL;
}

TEST_F(FiberTest, yield_single_thread) {
    fiber_t tid;
    ASSERT_EQ(0, fiber_start_background(&tid, NULL, yield_thread, NULL));
    ASSERT_EQ(0, fiber_join(tid, NULL));
}

} // namespace
