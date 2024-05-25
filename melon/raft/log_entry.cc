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

#include <melon/raft/log_entry.h>
#include <melon/proto/raft/local_storage.pb.h>

namespace melon::raft {

    melon::var::Adder<int64_t> g_nentries("raft_num_log_entries");

    LogEntry::LogEntry() : type(ENTRY_TYPE_UNKNOWN), peers(NULL), old_peers(NULL) {
        g_nentries << 1;
    }

    LogEntry::~LogEntry() {
        g_nentries << -1;
        delete peers;
        delete old_peers;
    }

    mutil::Status parse_configuration_meta(const mutil::IOBuf &data, LogEntry *entry) {
        mutil::Status status;
        ConfigurationPBMeta meta;
        mutil::IOBufAsZeroCopyInputStream wrapper(data);
        if (!meta.ParseFromZeroCopyStream(&wrapper)) {
            status.set_error(EINVAL, "Fail to parse ConfigurationPBMeta");
            return status;
        }
        entry->peers = new std::vector<PeerId>;
        for (int j = 0; j < meta.peers_size(); ++j) {
            entry->peers->push_back(PeerId(meta.peers(j)));
        }
        if (meta.old_peers_size() > 0) {
            entry->old_peers = new std::vector<PeerId>;
            for (int i = 0; i < meta.old_peers_size(); i++) {
                entry->old_peers->push_back(PeerId(meta.old_peers(i)));
            }
        }
        return status;
    }

    mutil::Status serialize_configuration_meta(const LogEntry *entry, mutil::IOBuf &data) {
        mutil::Status status;
        ConfigurationPBMeta meta;
        for (size_t i = 0; i < entry->peers->size(); ++i) {
            meta.add_peers((*(entry->peers))[i].to_string());
        }
        if (entry->old_peers) {
            for (size_t i = 0; i < entry->old_peers->size(); ++i) {
                meta.add_old_peers((*(entry->old_peers))[i].to_string());
            }
        }
        mutil::IOBufAsZeroCopyOutputStream wrapper(&data);
        if (!meta.SerializeToZeroCopyStream(&wrapper)) {
            status.set_error(EINVAL, "Fail to serialize ConfigurationPBMeta");
        }
        return status;
    }

}
