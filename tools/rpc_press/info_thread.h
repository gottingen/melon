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

#ifndef MELON_RPC_RPC_REPLAY_INFO_THREAD_H_
#define MELON_RPC_RPC_REPLAY_INFO_THREAD_H_

#include <pthread.h>
#include <melon/metrics/all.h>

namespace melon::rpc {

    struct InfoThreadOptions {
        melon::LatencyRecorder *latency_recorder;
        melon::gauge<int64_t> *sent_count;
        melon::gauge<int64_t> *error_count;

        InfoThreadOptions()
                : latency_recorder(nullptr), sent_count(nullptr), error_count(nullptr) {}
    };

    class InfoThread {
    public:
        InfoThread();

        ~InfoThread();

        void run();

        bool start(const InfoThreadOptions &);

        void stop();

    private:
        bool _stop;
        InfoThreadOptions _options;
        pthread_mutex_t _mutex;
        pthread_cond_t _cond;
        pthread_t _tid;
    };

} // namespace melon::rpc

#endif // MELON_RPC_RPC_REPLAY_INFO_THREAD_H_