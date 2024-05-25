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

#pragma once

#include <melon/raft/raft.h>

namespace melon::raft {
    namespace cli {

        struct CliOptions {
            int timeout_ms;
            int max_retry;

            CliOptions() : timeout_ms(-1), max_retry(3) {}
        };

        // Add a new peer into the replicating group which consists of |conf|.
        // Returns OK on success, error information otherwise.
        mutil::Status add_peer(const GroupId &group_id, const Configuration &conf,
                               const PeerId &peer_id, const CliOptions &options);

        // Remove a peer from the replicating group which consists of |conf|.
        // Returns OK on success, error information otherwise.
        mutil::Status remove_peer(const GroupId &group_id, const Configuration &conf,
                                  const PeerId &peer_id, const CliOptions &options);

    // Gracefully change the peers of the replication group.
        mutil::Status change_peers(const GroupId &group_id, const Configuration &conf,
                                   const Configuration &new_peers,
                                   const CliOptions &options);

        // Transfer the leader of the replication group to the target peer
        mutil::Status transfer_leader(const GroupId &group_id, const Configuration &conf,
                                      const PeerId &peer, const CliOptions &options);

        // Reset the peer set of the target peer
        mutil::Status reset_peer(const GroupId &group_id, const PeerId &peer_id,
                                 const Configuration &new_conf,
                                 const CliOptions &options);

        // Ask the peer to dump a snapshot immediately.
        mutil::Status snapshot(const GroupId &group_id, const PeerId &peer_id,
                               const CliOptions &options);

    }  // namespace cli
}  //  namespace melon::raft
