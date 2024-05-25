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


#include <melon/fiber/unstable.h>
#include <melon/raft/closure_queue.h>
#include <melon/raft/raft.h>

namespace melon::raft {

    ClosureQueue::ClosureQueue(bool usercode_in_pthread)
            : _first_index(0), _usercode_in_pthread(usercode_in_pthread) {}

    ClosureQueue::~ClosureQueue() {
        clear();
    }

    void ClosureQueue::clear() {
        std::deque<Closure *> saved_queue;
        {
            MELON_SCOPED_LOCK(_mutex);
            saved_queue.swap(_queue);
            _first_index = 0;
        }
        bool run_fiber = false;
        for (std::deque<Closure *>::iterator
                     it = saved_queue.begin(); it != saved_queue.end(); ++it) {
            if (*it) {
                (*it)->status().set_error(EPERM, "leader stepped down");
                run_closure_in_fiber_nosig(*it, _usercode_in_pthread);
                run_fiber = true;
            }
        }
        if (run_fiber) {
            fiber_flush();
        }
    }

    void ClosureQueue::reset_first_index(int64_t first_index) {
        MELON_SCOPED_LOCK(_mutex);
        MCHECK(_queue.empty());
        _first_index = first_index;
    }

    void ClosureQueue::append_pending_closure(Closure *c) {
        MELON_SCOPED_LOCK(_mutex);
        _queue.push_back(c);
    }

    int ClosureQueue::pop_closure_until(int64_t index,
                                        std::vector<Closure *> *out, int64_t *out_first_index) {
        out->clear();
        MELON_SCOPED_LOCK(_mutex);
        if (_queue.empty() || index < _first_index) {
            *out_first_index = index + 1;
            return 0;
        }
        if (index > _first_index + (int64_t) _queue.size() - 1) {
            MCHECK(false) << "Invalid index=" << index
                         << " _first_index=" << _first_index
                         << " _closure_queue_size=" << _queue.size();
            return -1;
        }
        *out_first_index = _first_index;
        for (int64_t i = _first_index; i <= index; ++i) {
            out->push_back(_queue.front());
            _queue.pop_front();
        }
        _first_index = index + 1;
        return 0;
    }

} //  namespace melon::raft
