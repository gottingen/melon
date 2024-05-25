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
