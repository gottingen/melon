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


#include <gtest/gtest.h>
#include <pthread.h>
#include <sched.h>
#include <unistd.h>
#include <stdio.h>
#include <melon/fiber/processor.h>

namespace {
volatile bool stop = false;

void* spinner(void*) {
    long counter = 0;
    for (; !stop; ++counter) {
        cpu_relax();
    }
    printf("spinned %ld\n", counter);
    return NULL;
}

void* yielder(void*) {
    int counter = 0;
    for (; !stop; ++counter) {
        sched_yield();
    }
    printf("sched_yield %d\n", counter);
    return NULL;
}

TEST(SchedYieldTest, sched_yield_when_all_core_busy) {
    stop = false;
    const int kNumCores = sysconf(_SC_NPROCESSORS_ONLN);
    ASSERT_TRUE(kNumCores > 0);
    pthread_t th0;
    pthread_create(&th0, NULL, yielder, NULL);
    
    pthread_t th[kNumCores];
    for (int i = 0; i < kNumCores; ++i) {
        pthread_create(&th[i], NULL, spinner, NULL);
    }
    sleep(1);
    stop = true;
    for (int i = 0; i < kNumCores; ++i) {
        pthread_join(th[i], NULL);
    }
    pthread_join(th0, NULL);
}
} // namespace
