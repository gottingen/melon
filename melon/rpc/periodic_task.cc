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



#include <melon/fiber/fiber.h>
#include <melon/fiber/unstable.h>
#include <melon/rpc/periodic_task.h>

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
        LOG(ERROR) << "Fail to start PeriodicTaskThread";
        static_cast<PeriodicTask*>(arg)->OnDestroyingTask();
        return;
    }
}

void PeriodicTaskManager::StartTaskAt(PeriodicTask* task, const timespec& abstime) {
    if (task == NULL) {
        LOG(ERROR) << "Param[task] is NULL";
        return;
    }
    fiber_timer_t timer_id;
    const int rc = fiber_timer_add(
        &timer_id, abstime, RunPeriodicTaskThread, task);
    if (rc != 0) {
        LOG(ERROR) << "Fail to add timer for RunPerodicTaskThread";
        task->OnDestroyingTask();
        return;
    }
}

} // namespace melon
