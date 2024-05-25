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
