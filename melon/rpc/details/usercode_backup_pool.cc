// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.


#include <deque>
#include <vector>
#include <gflags/gflags.h>
#include "melon/base/scoped_lock.h"
#include "melon/rpc/details/usercode_backup_pool.h"

namespace melon::fiber_internal {
// Defined in fiber/task_control.cpp
    void run_worker_startfn();
}


namespace melon::rpc {

    DEFINE_int32(usercode_backup_threads, 5, "# of backup threads to run user code"
                                             " when too many pthread worker of fibers are used");
    DEFINE_int32(max_pending_in_each_backup_thread, 10,
                 "Max number of un-run user code in each backup thread, requests"
                 " still coming in will be failed");

    // Store pending user code.
    struct UserCode {
        void (*fn)(void *);

        void *arg;
    };

    struct UserCodeBackupPool {
        // Run user code when parallelism of user code reaches the threshold
        std::deque<UserCode> queue;
        melon::status_gauge<int> inplace_var;
        melon::status_gauge<size_t> queue_size_var;
        melon::gauge<size_t> inpool_count;
        melon::per_second<melon::gauge<size_t> > inpool_per_second;
        // NOTE: we don't use Adder<double> directly which does not compile in gcc 3.4
        melon::gauge<int64_t> inpool_elapse_us;
        melon::status_gauge<double> inpool_elapse_s;
        melon::per_second<melon::status_gauge<double> > pool_usage;

        UserCodeBackupPool();

        int Init();

        void UserCodeRunningLoop();
    };

    static pthread_mutex_t s_usercode_mutex = PTHREAD_MUTEX_INITIALIZER;
    static pthread_cond_t s_usercode_cond = PTHREAD_COND_INITIALIZER;
    static pthread_once_t s_usercode_init = PTHREAD_ONCE_INIT;
    melon::static_atomic<int> g_usercode_inplace = MELON_STATIC_ATOMIC_INIT(0);
    bool g_too_many_usercode = false;
    static UserCodeBackupPool *s_usercode_pool = nullptr;

    static int GetUserCodeInPlace(void *) {
        return g_usercode_inplace.load(std::memory_order_relaxed);
    }

    static size_t GetUserCodeQueueSize(void *) {
        MELON_SCOPED_LOCK(s_usercode_mutex);
        return (s_usercode_pool != nullptr ? s_usercode_pool->queue.size() : 0);
    }

    static double GetInPoolElapseInSecond(void *arg) {
        return static_cast<melon::gauge<int64_t> *>(arg)->get_value() / 1000000.0;
    }

    UserCodeBackupPool::UserCodeBackupPool()
            : inplace_var("rpc_usercode_inplace", GetUserCodeInPlace, nullptr),
              queue_size_var("rpc_usercode_queue_size", GetUserCodeQueueSize, nullptr),
              inpool_count("rpc_usercode_backup_count"), inpool_per_second("rpc_usercode_backup_second", &inpool_count),
              inpool_elapse_s(GetInPoolElapseInSecond, &inpool_elapse_us),
              pool_usage("rpc_usercode_backup_usage", &inpool_elapse_s, 1) {
    }

    static void *UserCodeRunner(void *args) {
        static_cast<UserCodeBackupPool *>(args)->UserCodeRunningLoop();
        return nullptr;
    }

    int UserCodeBackupPool::Init() {
        // Like fiber workers, these threads never quit (to avoid potential hang
        // during termination of program).
        for (int i = 0; i < FLAGS_usercode_backup_threads; ++i) {
            pthread_t th;
            if (pthread_create(&th, nullptr, UserCodeRunner, this) != 0) {
                MELON_LOG(ERROR) << "Fail to create UserCodeRunner";
                return -1;
            }
        }
        return 0;
    }

    // Entry of backup thread for running user code.
    void UserCodeBackupPool::UserCodeRunningLoop() {
        melon::fiber_internal::run_worker_startfn();
        int64_t last_time = melon::get_current_time_micros();
        while (true) {
            bool blocked = false;
            UserCode usercode = {nullptr, nullptr};
            {
                MELON_SCOPED_LOCK(s_usercode_mutex);
                while (queue.empty()) {
                    pthread_cond_wait(&s_usercode_cond, &s_usercode_mutex);
                    blocked = true;
                }
                usercode = queue.front();
                queue.pop_front();
                if (g_too_many_usercode &&
                    (int) queue.size() <= FLAGS_usercode_backup_threads) {
                    g_too_many_usercode = false;
                }
            }
            const int64_t begin_time = (blocked ? melon::get_current_time_micros() : last_time);
            usercode.fn(usercode.arg);
            const int64_t end_time = melon::get_current_time_micros();
            inpool_count << 1;
            inpool_elapse_us << (end_time - begin_time);
            last_time = end_time;
        }
    }

    static void InitUserCodeBackupPool() {
        s_usercode_pool = new UserCodeBackupPool;
        if (s_usercode_pool->Init() != 0) {
            MELON_LOG(ERROR) << "Fail to init UserCodeBackupPool";
            // rare and critical, often happen when the program just started since
            // this function is called from GlobalInitializeOrDieImpl() as well,
            // quiting is the best choice.
            exit(1);
        }
    }

    void InitUserCodeBackupPoolOnceOrDie() {
        pthread_once(&s_usercode_init, InitUserCodeBackupPool);
    }

    void EndRunningUserCodeInPool(void (*fn)(void *), void *arg) {
        InitUserCodeBackupPoolOnceOrDie();

        g_usercode_inplace.fetch_sub(1, std::memory_order_relaxed);

        // Not enough idle workers, run the code in backup threads to prevent
        // all workers from being blocked and no responses will be processed
        // anymore (deadlocked).
        const UserCode usercode = {fn, arg};
        pthread_mutex_lock(&s_usercode_mutex);
        s_usercode_pool->queue.push_back(usercode);
        // If the queue has too many items, we can't drop the user code
        // directly which often must be run, for example: client-side done.
        // The solution is that we set a mark which is not cleared before
        // queue becomes short again. RPC code checks the mark before
        // submitting tasks that may generate more user code.
        if ((int) s_usercode_pool->queue.size() >=
            (FLAGS_usercode_backup_threads *
             FLAGS_max_pending_in_each_backup_thread)) {
            g_too_many_usercode = true;
        }
        pthread_mutex_unlock(&s_usercode_mutex);
        pthread_cond_signal(&s_usercode_cond);
    }

} // namespace melon::rpc