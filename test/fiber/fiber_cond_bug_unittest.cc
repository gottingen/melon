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


#include <atomic>
#include <iostream>
#include <mutex>
#include <string>
#include <unistd.h>

#include <gflags/gflags.h>
#include <gtest/gtest.h>

#include <melon/fiber/fiber.h>
#include "melon/fiber/condition_variable.h"
#include <melon/fiber/mutex.h>
#include <turbo/log/logging.h>
#include <melon/base/macros.h>
#include <melon/var/var.h>

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
                LOG(ERROR) << "producer thread:" << i << " stopped";
                return nullptr;
            }
            LOG(INFO) << "producer stat idx:" << i
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
    LOG(INFO) << "wait us:" << wait_us;
    int64_t idx = (int64_t)(arg);
    int32_t i = 0;
    while (!fiber_stopped(fiber_self())) {
        //LOG(INFO) << "come to a new round " << idx << "round[" << i << "]";
        {
            Lock lock(g_mutex); 
            while (g_que.size() >= g_capacity && !fiber_stopped(fiber_self())) {
                g_stat[idx].wait_count << 1;
                //LOG(INFO) << "wait begin " << idx;
                int ret = g_cond.wait_for(lock, wait_us);
                if (ret == ETIMEDOUT) {
                    g_stat[idx].wait_timeout_count << 1;
                    //LOG_EVERY_N_SEC(INFO, 1) << "wait timeout " << idx;
                } else {
                    g_stat[idx].wait_success_count << 1;
                    //LOG_EVERY_N_SEC(INFO, 1) << "wait early " << idx;
                }
            }
            g_que.push_back(++i);
            //LOG(INFO) << "push back " << idx << " data[" << i << "]";
        }
        usleep(rand() % 20 + 5);
        g_stat[idx].loop_count.fetch_add(1);
    }
    LOG(INFO) << "producer func return, idx:" << idx;
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
                LOG_EVERY_N_SEC(INFO, 1) << "pop a data";
            } else {
                LOG_EVERY_N_SEC(INFO, 1) << "que is empty";
            }
        }
        usleep(rand() % 300 + 500);
        if (need_notify) {
            //g_cond.notify_all();
            //LOG(WARNING) << "notify";
        }
    }
    LOG(INFO) << "consumer func return";
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
