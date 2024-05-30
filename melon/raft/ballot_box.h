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

#include <stdint.h>                             // int64_t
#include <set>                                  // std::set
#include <deque>
#include <melon/utility/atomicops.h>                     // mutil::atomic
#include <melon/raft/raft.h>
#include <melon/raft/util.h>
#include <melon/raft/ballot.h>

namespace melon::raft {

    class FSMCaller;

    class ClosureQueue;

    struct BallotBoxOptions {
        BallotBoxOptions()
                : waiter(NULL), closure_queue(NULL) {}

        FSMCaller *waiter;
        ClosureQueue *closure_queue;
    };

    struct BallotBoxStatus {
        BallotBoxStatus()
                : committed_index(0), pending_index(0), pending_queue_size(0) {}

        int64_t committed_index;
        int64_t pending_index;
        int64_t pending_queue_size;
    };

    class BallotBox {
    public:
        BallotBox();

        ~BallotBox();

        int init(const BallotBoxOptions &options);

        // Called by leader, otherwise the behavior is undefined
        // Set logs in [first_log_index, last_log_index] are stable at |peer|.
        int commit_at(int64_t first_log_index, int64_t last_log_index,
                      const PeerId &peer);

        // Called when the leader steps down, otherwise the behavior is undefined
        // When a leader steps down, the uncommitted user applications should
        // fail immediately, which the new leader will deal whether to commit or
        // truncate.
        int clear_pending_tasks();

        // Called when a candidate becomes the new leader, otherwise the behavior is
        // undefined.
        // According to the raft algorithm, the logs from pervious terms can't be
        // committed until a log at the new term becomes committed, so
        // |new_pending_index| should be |last_log_index| + 1.
        int reset_pending_index(int64_t new_pending_index);

        // Called by leader, otherwise the behavior is undefined
        // Store application context before replication.
        int append_pending_task(const Configuration &conf,
                                const Configuration *old_conf,
                                Closure *closure);

        // Called by follower, otherwise the behavior is undefined.
        // Set committed index received from leader
        int set_last_committed_index(int64_t last_committed_index);

        int64_t last_committed_index() { return _last_committed_index.load(mutil::memory_order_acquire); }

        void describe(std::ostream &os, bool use_html);

        void get_status(BallotBoxStatus *ballot_box_status);

    private:

        FSMCaller *_waiter;
        ClosureQueue *_closure_queue;
        raft_mutex_t _mutex;
        mutil::atomic<int64_t> _last_committed_index;
        int64_t _pending_index;
        std::deque<Ballot> _pending_meta_queue;

    };

}  //  namespace melon::raft
