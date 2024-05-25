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

#include <melon/raft/configuration.h>         // Configuration
#include <melon/raft/log_entry.h>             // LogId

namespace melon::raft {

    struct ConfigurationEntry {
        LogId id;
        Configuration conf;
        Configuration old_conf;

        ConfigurationEntry() = default;

        explicit ConfigurationEntry(const LogEntry &entry) {
            id = entry.id;
            conf = *(entry.peers);
            if (entry.old_peers) {
                old_conf = *(entry.old_peers);
            }
        }

        bool stable() const { return old_conf.empty(); }

        bool empty() const { return conf.empty(); }

        void list_peers(std::set<PeerId> *peers) {
            peers->clear();
            conf.append_peers(peers);
            old_conf.append_peers(peers);
        }

        bool contains(const PeerId &peer) const { return conf.contains(peer) || old_conf.contains(peer); }
    };

// Manager the history of configuration changing
    class ConfigurationManager {
    public:
        ConfigurationManager()  = default;

        ~ConfigurationManager()  = default;

        // add new configuration at index
        int add(const ConfigurationEntry &entry);

        // [1, first_index_kept) are being discarded
        void truncate_prefix(int64_t first_index_kept);

        // (last_index_kept, infinity) are being discarded
        void truncate_suffix(int64_t last_index_kept);

        void set_snapshot(const ConfigurationEntry &snapshot);

        void get(int64_t last_included_index, ConfigurationEntry *entry);

        const ConfigurationEntry &last_configuration() const;

    private:

        std::deque<ConfigurationEntry> _configurations;
        ConfigurationEntry _snapshot;
    };

}  //  namespace melon::raft
