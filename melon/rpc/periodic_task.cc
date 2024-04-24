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



#include <melon/fiber/fiber.h>
#include <melon/fiber/unstable.h>
#include "melon/rpc/periodic_task.h"

namespace melon {

PeriodicTask::~PeriodicTask() {
}

static void* PeriodicTaskThread(void* arg) {
    PeriodicTask* task = static_cast<PeriodicTask*>(arg);
    timespec abstime;
    if (!task->OnTriggeringTask(&abstime)) { // end
        task->OnDestroyingTask();
        return NULL;
    }
    PeriodicTaskManager::StartTaskAt(task, abstime);
    return NULL;
}

static void RunPeriodicTaskThread(void* arg) {
    fiber_t th = 0;
    int rc = fiber_start_background(
        &th, &FIBER_ATTR_NORMAL, PeriodicTaskThread, arg);
    if (rc != 0) {
        MLOG(ERROR) << "Fail to start PeriodicTaskThread";
        static_cast<PeriodicTask*>(arg)->OnDestroyingTask();
        return;
    }
}

void PeriodicTaskManager::StartTaskAt(PeriodicTask* task, const timespec& abstime) {
    if (task == NULL) {
        MLOG(ERROR) << "Param[task] is NULL";
        return;
    }
    fiber_timer_t timer_id;
    const int rc = fiber_timer_add(
        &timer_id, abstime, RunPeriodicTaskThread, task);
    if (rc != 0) {
        MLOG(ERROR) << "Fail to add timer for RunPerodicTaskThread";
        task->OnDestroyingTask();
        return;
    }
}

} // namespace melon
