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



#ifndef MELON_RPC_PERIODIC_TASK_H_
#define MELON_RPC_PERIODIC_TASK_H_

namespace melon {

// Override OnTriggeringTask() with code that needs to be periodically run. If
// the task is completed, the method should return false; Otherwise the method
// should return true and set `next_abstime' to the time that the task should
// be run next time.
// Each call to OnTriggeringTask() is run in a separated fiber which can be
// suspended. To preserve states between different calls, put the states as
// fields of (subclass of) PeriodicTask.
// If any error occurs or OnTriggeringTask() returns false, the task is called
// with OnDestroyingTask() and will not be scheduled anymore.
class PeriodicTask {
public:
    virtual ~PeriodicTask();
    virtual bool OnTriggeringTask(timespec* next_abstime) = 0;
    virtual void OnDestroyingTask() = 0;
};

class PeriodicTaskManager {
public:
    static void StartTaskAt(PeriodicTask* task, const timespec& abstime);
};


} // namespace melon

#endif  // MELON_RPC_PERIODIC_TASK_H_
