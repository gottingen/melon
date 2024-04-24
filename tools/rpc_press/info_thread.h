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

#pragma once

#include <pthread.h>
#include <melon/var/var.h>

namespace melon {

struct InfoThreadOptions {
    melon::var::LatencyRecorder* latency_recorder;
    melon::var::Adder<int64_t>* sent_count;
    melon::var::Adder<int64_t>* error_count;

    InfoThreadOptions()
        : latency_recorder(NULL)
        , sent_count(NULL)
        , error_count(NULL) {}
};

class InfoThread {
public:
    InfoThread();
    ~InfoThread();
    void run();
    bool start(const InfoThreadOptions&);
    void stop();
    
private:
    bool _stop;
    InfoThreadOptions _options;
    pthread_mutex_t _mutex;
    pthread_cond_t _cond;
    pthread_t _tid;
};

} // namespace melon
