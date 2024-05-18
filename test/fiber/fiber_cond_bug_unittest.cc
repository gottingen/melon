// Copyright 2023 The Elastic-AI Authors.
// part of Elastic AI Search
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//


#include <atomic>
#include <iostream>
#include <mutex>
#include <string>
#include <unistd.h>

#include <gflags/gflags.h>
#include <gtest/gtest.h>

#include "melon/fiber/fiber.h"
#include "melon/fiber/condition_variable.h"
#include "melon/fiber/mutex.h"
#include "melon/utility/logging.h"
#include "melon/utility/macros.h"
#include "melon/var/var.h"

DEFINE_int64(wait_us, 5, "wait us");
typedef std::unique_lock<fiber::Mutex> Lock;
typedef fiber::ConditionVariable Condition;
fiber::Mutex      g_mutex;
Condition           g_cond;
std::deque<int32_t> g_que;
const size_t        g_capacity = 2000;
const int PRODUCER_NUM = 5;
struct ProducerStat {
    std::atomic<int> loop_count;
    melon::var::Adder<int> wait_count;
    melon::var::Adder<int> wait_timeout_count;
    melon::var::Adder<int> wait_success_count;
};
ProducerStat g_stat[PRODUCER_NUM];

void* print_func(void* arg) {
    int last_loop[PRODUCER_NUM] = {0};
    for (int j = 0; j < 10; j++) {
        usleep(1000000);
        for (int i = 0; i < PRODUCER_NUM; i++) {
            if (g_stat[i].loop_count.load() <= last_loop[i]) {
                MLOG(ERROR) << "producer thread:" << i << " stopped";
                return nullptr;
            }
            MLOG(INFO) << "producer stat idx:" << i
                      << " wait:" << g_stat[i].wait_count
                      << " wait_timeout:" << g_stat[i].wait_timeout_count
                      << " wait_success:" << g_stat[i].wait_success_count;
            g_stat[i].loop_count = g_stat[i].loop_count.load();
        }
    }
    return (void*)1;
}

void* produce_func(void* arg) {
    const int64_t wait_us = FLAGS_wait_us;
    MLOG(INFO) << "wait us:" << wait_us;
    int64_t idx = (int64_t)(arg);
    int32_t i = 0;
    while (!fiber_stopped(fiber_self())) {
        //MLOG(INFO) << "come to a new round " << idx << "round[" << i << "]";
        {
            Lock lock(g_mutex); 
            while (g_que.size() >= g_capacity && !fiber_stopped(fiber_self())) {
                g_stat[idx].wait_count << 1;
                //MLOG(INFO) << "wait begin " << idx;
                int ret = g_cond.wait_for(lock, wait_us);
                if (ret == ETIMEDOUT) {
                    g_stat[idx].wait_timeout_count << 1;
                    //MLOG_EVERY_SECOND(INFO) << "wait timeout " << idx;
                } else {
                    g_stat[idx].wait_success_count << 1;
                    //MLOG_EVERY_SECOND(INFO) << "wait early " << idx;
                }
            }
            g_que.push_back(++i);
            //MLOG(INFO) << "push back " << idx << " data[" << i << "]";
        }
        usleep(rand() % 20 + 5);
        g_stat[idx].loop_count.fetch_add(1);
    }
    MLOG(INFO) << "producer func return, idx:" << idx;
    return nullptr;
}

void* consume_func(void* arg) {
    while (!fiber_stopped(fiber_self())) {
        bool need_notify = false;
        {
            Lock lock(g_mutex);
            need_notify = (g_que.size() == g_capacity);
            if (!g_que.empty()) {
                g_que.pop_front();
                MLOG_EVERY_SECOND(INFO) << "pop a data";
            } else {
                MLOG_EVERY_SECOND(INFO) << "que is empty";
            }
        }
        usleep(rand() % 300 + 500);
        if (need_notify) {
            //g_cond.notify_all();
            //MLOG(WARNING) << "notify";
        }
    }
    MLOG(INFO) << "consumer func return";
    return nullptr;
}

TEST(FiberCondBugTest, test_bug) {
    fiber_t tids[PRODUCER_NUM];
    for (int i = 0; i < PRODUCER_NUM; i++) {
        fiber_start_background(&tids[i], NULL, produce_func, (void*)(int64_t)i);
    }
    fiber_t tid;
    fiber_start_background(&tid, NULL, consume_func, NULL);

    int64_t ret = (int64_t)print_func(nullptr);

    fiber_stop(tid);
    fiber_join(tid, nullptr);
    for (int i = 0; i < PRODUCER_NUM; i++) {
        fiber_stop(tids[i]);
        fiber_join(tids[i], nullptr);
    }

    ASSERT_EQ(ret, 1);
}