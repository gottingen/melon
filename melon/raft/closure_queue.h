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


#ifndef  MELON_RAFT_CLOSURE_QUEUE_H_
#define  MELON_RAFT_CLOSURE_QUEUE_H_

#include "melon/raft/util.h"

namespace melon::raft {

    // Holding the closure waiting for the commitment of logs
    class ClosureQueue {
    public:
        explicit ClosureQueue(bool usercode_in_pthread);

        ~ClosureQueue();

        // Clear all the pending closure and run done
        void clear();

        // Called when a candidate becomes the new leader, otherwise the behavior is
        // undefined.
        // Reset the first index of the coming pending closures to |first_index|
        void reset_first_index(int64_t first_index);

        // Called by leader, otherwise the behavior is undefined
        // Append the closure
        void append_pending_closure(Closure *c);

        // Pop all the closure until |index| (included) into out in the same order
        // of their indexes, |out_first_index| would be assigned the index of out[0] if
        // out is not empty, index + 1 otherwise.
        int pop_closure_until(int64_t index,
                              std::vector<Closure *> *out, int64_t *out_first_index);

    private:
        // TODO: a spsc lock-free queue would help
        raft_mutex_t _mutex;
        int64_t _first_index;
        std::deque<Closure *> _queue;
        bool _usercode_in_pthread;

    };

} //  namespace melon::raft

#endif  // MELON_RAFT_CLOSURE_QUEUE_H_
