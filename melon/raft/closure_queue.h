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

#include <melon/raft/util.h>

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
