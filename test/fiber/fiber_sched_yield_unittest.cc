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
