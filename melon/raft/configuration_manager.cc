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

#include <melon/raft/configuration_manager.h>

namespace melon::raft {

    int ConfigurationManager::add(const ConfigurationEntry &entry) {
        if (!_configurations.empty()) {
            if (_configurations.back().id.index >= entry.id.index) {
                MCHECK(false) << "Did you forget to call truncate_suffix before "
                                " the last log index goes back";
                return -1;
            }
        }
        _configurations.push_back(entry);
        return 0;
    }

    void ConfigurationManager::truncate_prefix(const int64_t first_index_kept) {
        while (!_configurations.empty()
               && _configurations.front().id.index < first_index_kept) {
            _configurations.pop_front();
        }
    }

    void ConfigurationManager::truncate_suffix(const int64_t last_index_kept) {
        while (!_configurations.empty()
               && _configurations.back().id.index > last_index_kept) {
            _configurations.pop_back();
        }
    }

    void ConfigurationManager::set_snapshot(const ConfigurationEntry &entry) {
        MCHECK_GE(entry.id, _snapshot.id);
        _snapshot = entry;
    }

    void ConfigurationManager::get(int64_t last_included_index,
                                   ConfigurationEntry *conf) {
        if (_configurations.empty()) {
            MCHECK_GE(last_included_index, _snapshot.id.index);
            *conf = _snapshot;
            return;
        }
        std::deque<ConfigurationEntry>::iterator it;
        for (it = _configurations.begin(); it != _configurations.end(); ++it) {
            if (it->id.index > last_included_index) {
                break;
            }
        }
        if (it == _configurations.begin()) {
            *conf = _snapshot;
            return;
        }
        --it;
        *conf = *it;
    }

    const ConfigurationEntry &ConfigurationManager::last_configuration() const {
        if (!_configurations.empty()) {
            return _configurations.back();
        }
        return _snapshot;
    }

}  //  namespace melon::raft
