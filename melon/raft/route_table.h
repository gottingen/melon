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

#include <melon/raft/configuration.h>                 // Configuration
#include <melon/raft/raft.h>

// Maintain routes to raft groups

namespace melon::raft {
    namespace rtb {

        // Update configuration of group in route table
        int update_configuration(const GroupId &group, const Configuration &conf);

        int update_configuration(const GroupId &group, const std::string &conf_str);

        // Get the cached leader of group.
        // Returns:
        //  0 : success
        //  1 : Not sure about the leader
        //  -1, otherwise
        int select_leader(const GroupId &group, PeerId *leader);

        // Update leader
        int update_leader(const GroupId &group, const PeerId &leader);

        int update_leader(const GroupId &group, const std::string &leader_str);

            // Blocking the thread until query_leader finishes
        mutil::Status refresh_leader(const GroupId &group, int timeout_ms);

        // Remove this group from route table
        int remove_group(const GroupId &group);

    }  // namespace rtb
}  // namespace melon::raft
