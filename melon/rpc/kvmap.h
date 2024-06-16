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


#ifndef MELON_RPC_KVMAP_H_
#define MELON_RPC_KVMAP_H_

#include <melon/base/containers/flat_map.h>

namespace melon {
    
// Remember Key/Values in string
class KVMap {
public:
    typedef mutil::FlatMap<std::string, std::string> Map;
    typedef Map::const_iterator Iterator;

    KVMap() {}

    // Exchange internal fields with another KVMap.
    void Swap(KVMap &rhs) { _entries.swap(rhs._entries); }

    // Reset internal fields as if they're just default-constructed.
    void Clear() { _entries.clear(); }

    // Get value of a key(case-sensitive)
    // Return pointer to the value, NULL on not found.
    const std::string* Get(const char* key) const { return _entries.seek(key); }
    const std::string* Get(const std::string& key) const { return _entries.seek(key); }

    // Set value of a key
    void Set(const std::string& key, const std::string& value) { GetOrAdd(key) = value; }
    void Set(const std::string& key, const char* value) { GetOrAdd(key) = value; }
    // Convert other types to string as well
    template <typename T>
    void Set(const std::string& key, const T& value) { GetOrAdd(key) = std::to_string(value); }

    // Remove a key
    void Remove(const char* key) { _entries.erase(key); }
    void Remove(const std::string& key) { _entries.erase(key); }

    // Get iterators to iterate key/value
    Iterator Begin() const { return _entries.begin(); }
    Iterator End() const { return _entries.end(); }
    
    // number of key/values
    size_t Count() const { return _entries.size(); }

private:
    std::string& GetOrAdd(const std::string& key) {
        if (!_entries.initialized()) {
            _entries.init(29);
        }
        return _entries[key];
    }

    Map _entries;
};

} // namespace melon

#endif // MELON_RPC_KVMAP_H_
