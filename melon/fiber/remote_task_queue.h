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


#pragma once

#include <melon/base/bounded_queue.h>
#include <melon/base/macros.h>

namespace fiber {

    class TaskGroup;

    // A queue for storing fibers created by non-workers. Since non-workers
    // randomly choose a TaskGroup to push which distributes the contentions,
    // this queue is simply implemented as a queue protected with a lock.
    // The function names should be self-explanatory.
    class RemoteTaskQueue {
    public:
        RemoteTaskQueue() {}

        int init(size_t cap) {
            const size_t memsize = sizeof(fiber_t) * cap;
            void* q_mem = malloc(memsize);
            if (q_mem == NULL) {
                return -1;
            }
            mutil::BoundedQueue<fiber_t> q(q_mem, memsize, mutil::OWNS_STORAGE);
            _tasks.swap(q);
            return 0;
        }

        bool pop(fiber_t* task) {
            if (_tasks.empty()) {
                return false;
            }
            _mutex.lock();
            const bool result = _tasks.pop(task);
            _mutex.unlock();
            return result;
        }

        bool push(fiber_t task) {
            _mutex.lock();
            const bool res = push_locked(task);
            _mutex.unlock();
            return res;
        }

        bool push_locked(fiber_t task) {
            return _tasks.push(task);
        }

        size_t capacity() const { return _tasks.capacity(); }

    private:
    friend class TaskGroup;
        DISALLOW_COPY_AND_ASSIGN(RemoteTaskQueue);
        mutil::BoundedQueue<fiber_t> _tasks;
        std::mutex _mutex;
    };

}  // namespace fiber
