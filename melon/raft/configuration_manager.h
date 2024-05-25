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
