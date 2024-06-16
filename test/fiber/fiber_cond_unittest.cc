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


#include <map>
#include <gtest/gtest.h>
#include <melon/utility/atomicops.h>
#include <melon/utility/time.h>
#include <melon/base/macros.h>
#include <melon/base/scoped_lock.h>
#include <melon/utility/gperftools_profiler.h>
#include <melon/fiber/fiber.h>
#include "melon/fiber/condition_variable.h"
#include <melon/fiber/stack.h>
#include <cinttypes>

namespace {
struct Arg {
    fiber_mutex_t m;
    fiber_cond_t c;
};

pthread_mutex_t wake_mutex = PTHREAD_MUTEX_INITIALIZER;
long signal_start_time = 0;
std::vector<fiber_t> wake_tid;
std::vector<long> wake_time;
volatile bool stop = false;
const long SIGNAL_INTERVAL_US = 10000;

void* signaler(void* void_arg) {
    Arg* a = (Arg*)void_arg;
    signal_start_time = mutil::gettimeofday_us();
    while (!stop) {
        fiber_usleep(SIGNAL_INTERVAL_US);
        fiber_cond_signal(&a->c);
    }
    return NULL;
}

void* waiter(void* void_arg) {
    Arg* a = (Arg*)void_arg;
    fiber_mutex_lock(&a->m);
    while (!stop) {
        fiber_cond_wait(&a->c, &a->m);
        
        MELON_SCOPED_LOCK(wake_mutex);
        wake_tid.push_back(fiber_self());
        wake_time.push_back(mutil::gettimeofday_us());
    }
    fiber_mutex_unlock(&a->m);
    return NULL;
}

TEST(CondTest, sanity) {
    Arg a;
    ASSERT_EQ(0, fiber_mutex_init(&a.m, NULL));
    ASSERT_EQ(0, fiber_cond_init(&a.c, NULL));
    // has no effect
    ASSERT_EQ(0, fiber_cond_signal(&a.c));

    stop = false;
    wake_tid.resize(1024);
    wake_tid.clear();
    wake_time.resize(1024);
    wake_time.clear();
    
    fiber_t wth[8];
    const size_t NW = ARRAY_SIZE(wth);
    for (size_t i = 0; i < NW; ++i) {
        ASSERT_EQ(0, fiber_start_urgent(&wth[i], NULL, waiter, &a));
    }
    
    fiber_t sth;
    ASSERT_EQ(0, fiber_start_urgent(&sth, NULL, signaler, &a));

    fiber_usleep(SIGNAL_INTERVAL_US * 200);

    pthread_mutex_lock(&wake_mutex);
    const size_t nbeforestop = wake_time.size();
    pthread_mutex_unlock(&wake_mutex);
    
    stop = true;
    for (size_t i = 0; i < NW; ++i) {
        fiber_cond_signal(&a.c);
    }
    
    fiber_join(sth, NULL);
    for (size_t i = 0; i < NW; ++i) {
        fiber_join(wth[i], NULL);
    }

    printf("wake up for %lu times\n", wake_tid.size());

    // Check timing
    long square_sum = 0;
    for (size_t i = 0; i < nbeforestop; ++i) {
        long last_time = (i ? wake_time[i-1] : signal_start_time);
        long delta = wake_time[i] - last_time - SIGNAL_INTERVAL_US;
        EXPECT_GT(wake_time[i], last_time);
        square_sum += delta * delta;
        EXPECT_LT(labs(delta), 10000L) << "error[" << i << "]=" << delta << "="
            << wake_time[i] << " - " << last_time;
    }
    printf("Average error is %fus\n", sqrt(square_sum / std::max(nbeforestop, 1UL)));

    // Check fairness
    std::map<fiber_t, int> count;
    for (size_t i = 0; i < wake_tid.size(); ++i) {
        ++count[wake_tid[i]];
    }
    EXPECT_EQ(NW, count.size());
    int avg_count = (int)(wake_tid.size() / count.size());
    for (std::map<fiber_t, int>::iterator
             it = count.begin(); it != count.end(); ++it) {
        ASSERT_LE(abs(it->second - avg_count), 1)
            << "fiber=" << it->first
            << " count=" << it->second
            << " avg=" << avg_count;
        printf("%" PRId64 " wakes up %d times\n", it->first, it->second);
    }

    fiber_cond_destroy(&a.c);
    fiber_mutex_destroy(&a.m);
}

struct WrapperArg {
    fiber::Mutex mutex;
    fiber::ConditionVariable cond;
};

void* cv_signaler(void* void_arg) {
    WrapperArg* a = (WrapperArg*)void_arg;
    signal_start_time = mutil::gettimeofday_us();
    while (!stop) {
        fiber_usleep(SIGNAL_INTERVAL_US);
        a->cond.notify_one();
    }
    return NULL;
}

void* cv_bmutex_waiter(void* void_arg) {
    WrapperArg* a = (WrapperArg*)void_arg;
    std::unique_lock<fiber_mutex_t> lck(*a->mutex.native_handler());
    while (!stop) {
        a->cond.wait(lck);
    }
    return NULL;
}

void* cv_mutex_waiter(void* void_arg) {
    WrapperArg* a = (WrapperArg*)void_arg;
    std::unique_lock<fiber::Mutex> lck(a->mutex);
    while (!stop) {
        a->cond.wait(lck);
    }
    return NULL;
}

#define COND_IN_PTHREAD

#ifndef COND_IN_PTHREAD
#define pthread_join fiber_join
#define pthread_create fiber_start_urgent
#endif

TEST(CondTest, cpp_wrapper) {
    stop = false;
    fiber::ConditionVariable cond;
    pthread_t bmutex_waiter_threads[8];
    pthread_t mutex_waiter_threads[8];
    pthread_t signal_thread;
    WrapperArg a;
    for (size_t i = 0; i < ARRAY_SIZE(bmutex_waiter_threads); ++i) {
        ASSERT_EQ(0, pthread_create(&bmutex_waiter_threads[i], NULL,
                                    cv_bmutex_waiter, &a));
        ASSERT_EQ(0, pthread_create(&mutex_waiter_threads[i], NULL,
                                    cv_mutex_waiter, &a));
    }
    ASSERT_EQ(0, pthread_create(&signal_thread, NULL, cv_signaler, &a));
    fiber_usleep(100L * 1000);
    {
        MELON_SCOPED_LOCK(a.mutex);
        stop = true;
    }
    pthread_join(signal_thread, NULL);
    a.cond.notify_all();
    for (size_t i = 0; i < ARRAY_SIZE(bmutex_waiter_threads); ++i) {
        pthread_join(bmutex_waiter_threads[i], NULL);
        pthread_join(mutex_waiter_threads[i], NULL);
    }
}

#ifndef COND_IN_PTHREAD
#undef pthread_join
#undef pthread_create
#endif

class Signal {
protected:
    Signal() : _signal(0) {}
    void notify() {
        MELON_SCOPED_LOCK(_m);
        ++_signal;
        _c.notify_one();
    }

    int wait(int old_signal) {
        std::unique_lock<fiber::Mutex> lck(_m);
        while (_signal == old_signal) {
            _c.wait(lck);
        }
        return _signal;
    }

private:
    fiber::Mutex _m;
    fiber::ConditionVariable _c;
    int _signal;
};

struct PingPongArg {
    bool stopped;
    Signal sig1;
    Signal sig2;
    std::atomic<int> nthread;
    std::atomic<long> total_count;
};

void *ping_pong_thread(void* arg) {
    PingPongArg* a = (PingPongArg*)arg;
    long local_count = 0;
    bool odd = (a->nthread.fetch_add(1)) % 2;
    int old_signal = 0;
    while (!a->stopped) {
        if (odd) {
            a->sig1.notify();
            old_signal = a->sig2.wait(old_signal);
        } else {
            old_signal = a->sig1.wait(old_signal);
            a->sig2.notify();
        }
        ++local_count;
    }
    a->total_count.fetch_add(local_count);
    return NULL;
}

TEST(CondTest, ping_pong) {
    PingPongArg arg;
    arg.stopped = false;
    arg.nthread = 0;
    fiber_t threads[2];
    ProfilerStart("cond.prof");
    for (int i = 0; i < 2; ++i) {
        ASSERT_EQ(0, fiber_start_urgent(&threads[i], NULL, ping_pong_thread, &arg));
    }
    usleep(1000 * 1000);
    arg.stopped = true;
    arg.sig1.notify();
    arg.sig2.notify();
    for (int i = 0; i < 2; ++i) {
        ASSERT_EQ(0, fiber_join(threads[i], NULL));
    }
    ProfilerStop();
    LOG(INFO) << "total_count=" << arg.total_count.load();
}

struct BroadcastArg {
    fiber::ConditionVariable wait_cond;
    fiber::ConditionVariable broadcast_cond;
    fiber::Mutex mutex;
    int nwaiter;
    int cur_waiter;
    int rounds;
    int sig;
};

void* wait_thread(void* arg) {
    BroadcastArg* ba = (BroadcastArg*)arg;
    std::unique_lock<fiber::Mutex> lck(ba->mutex);
    while (ba->rounds > 0) {
        const int saved_round = ba->rounds;
        ++ba->cur_waiter;
        while (saved_round == ba->rounds) {
            if (ba->cur_waiter >= ba->nwaiter) {
                ba->broadcast_cond.notify_one();
            }
            ba->wait_cond.wait(lck);
        }
    }
    return NULL;
}

void* broadcast_thread(void* arg) {
    BroadcastArg* ba = (BroadcastArg*)arg;
    //int local_round = 0;
    while (ba->rounds > 0) {
        std::unique_lock<fiber::Mutex> lck(ba->mutex);
        while (ba->cur_waiter < ba->nwaiter) {
            ba->broadcast_cond.wait(lck);
        }
        ba->cur_waiter = 0;
        --ba->rounds;
        ba->wait_cond.notify_all();
    }
    return NULL;
}

void* disturb_thread(void* arg) {
    BroadcastArg* ba = (BroadcastArg*)arg;
    std::unique_lock<fiber::Mutex> lck(ba->mutex);
    while (ba->rounds > 0) {
        lck.unlock();
        lck.lock();
    }
    return NULL;
}

TEST(CondTest, mixed_usage) {
    BroadcastArg ba;
    ba.nwaiter = 0;
    ba.cur_waiter = 0;
    ba.rounds = 30000;
    const int NTHREADS = 10;
    ba.nwaiter = NTHREADS * 2;

    fiber_t normal_threads[NTHREADS];
    for (int i = 0; i < NTHREADS; ++i) {
        ASSERT_EQ(0, fiber_start_urgent(&normal_threads[i], NULL, wait_thread, &ba));
    }
    pthread_t pthreads[NTHREADS];
    for (int i = 0; i < NTHREADS; ++i) {
        ASSERT_EQ(0, pthread_create(&pthreads[i], NULL,
                                    wait_thread, &ba));
    }
    pthread_t broadcast;
    pthread_t disturb;
    ASSERT_EQ(0, pthread_create(&broadcast, NULL, broadcast_thread, &ba));
    ASSERT_EQ(0, pthread_create(&disturb, NULL, disturb_thread, &ba));
    for (int i = 0; i < NTHREADS; ++i) {
        fiber_join(normal_threads[i], NULL);
        pthread_join(pthreads[i], NULL);
    }
    pthread_join(broadcast, NULL);
    pthread_join(disturb, NULL);
}

class FiberCond {
public:
    FiberCond() {
        fiber_cond_init(&_cond, NULL);
        fiber_mutex_init(&_mutex, NULL);
        _count = 1;
    }
    ~FiberCond() {
        fiber_mutex_destroy(&_mutex);
        fiber_cond_destroy(&_cond);
    }

    void Init(int count = 1) {
        _count = count;
    }

    int Signal() {
        int ret = 0;
        fiber_mutex_lock(&_mutex);
        _count --;
        fiber_cond_signal(&_cond);
        fiber_mutex_unlock(&_mutex);
        return ret;
    }

    int Wait() {
        int ret = 0;
        fiber_mutex_lock(&_mutex);
        while (_count > 0) {
            ret = fiber_cond_wait(&_cond, &_mutex);
        }
        fiber_mutex_unlock(&_mutex);
        return ret;
    }
private:
    int _count;
    fiber_cond_t _cond;
    fiber_mutex_t _mutex;
};

volatile bool g_stop = false;
bool started_wait = false;
bool ended_wait = false;

void* usleep_thread(void *) {
    while (!g_stop) {
        fiber_usleep(1000L * 1000L);
    }
    return NULL;
}

void* wait_cond_thread(void* arg) {
    FiberCond* c = (FiberCond*)arg;
    started_wait = true;
    c->Wait();
    ended_wait = true;
    return NULL;
}

static void launch_many_fibers() {
    g_stop = false;
    fiber_t tid;
    FiberCond c;
    c.Init();
    mutil::Timer tm;
    fiber_start_urgent(&tid, &FIBER_ATTR_PTHREAD, wait_cond_thread, &c);
    std::vector<fiber_t> tids;
    tids.reserve(32768);
    tm.start();
    for (size_t i = 0; i < 32768; ++i) {
        fiber_t t0;
        ASSERT_EQ(0, fiber_start_background(&t0, NULL, usleep_thread, NULL));
        tids.push_back(t0);
    }
    tm.stop();
    LOG(INFO) << "Creating fibers took " << tm.u_elapsed() << " us";
    usleep(3 * 1000 * 1000L);
    c.Signal();
    g_stop = true;
    fiber_join(tid, NULL);
    for (size_t i = 0; i < tids.size(); ++i) {
        LOG_EVERY_N_SEC(INFO, 1) << "Joined " << i << " threads";
        fiber_join(tids[i], NULL);
    }
    LOG_EVERY_N_SEC(INFO, 1) << "Joined " << tids.size() << " threads";
}

TEST(CondTest, too_many_fibers_from_pthread) {
    launch_many_fibers();
}

static void* run_launch_many_fibers(void*) {
    launch_many_fibers();
    return NULL;
}

TEST(CondTest, too_many_fibers_from_fiber) {
    fiber_t th;
    ASSERT_EQ(0, fiber_start_urgent(&th, NULL, run_launch_many_fibers, NULL));
    fiber_join(th, NULL);
}
} // namespace
