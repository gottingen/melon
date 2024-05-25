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



#ifndef MELON_RPC_SERVER_ID_H_
#define MELON_RPC_SERVER_ID_H_


#include <vector>
#include <melon/utility/containers/hash_tables.h>   // hash
#include <melon/utility/containers/flat_map.h>
#include <melon/rpc/socket_id.h>

namespace melon {

    // Representing a server inside LoadBalancer.
    struct ServerId {
        ServerId() : id(0) {}

        explicit ServerId(SocketId id_in) : id(id_in) {}

        ServerId(SocketId id_in, const std::string &tag_in)
                : id(id_in), tag(tag_in) {}

        SocketId id;
        std::string tag;
    };

    inline bool operator==(const ServerId &id1, const ServerId &id2) { return id1.id == id2.id && id1.tag == id2.tag; }

    inline bool operator!=(const ServerId &id1, const ServerId &id2) { return !(id1 == id2); }

    inline bool operator<(const ServerId &id1, const ServerId &id2) {
        return id1.id != id2.id ? (id1.id < id2.id) : (id1.tag < id2.tag);
    }

    inline std::ostream &operator<<(std::ostream &os, const ServerId &tsid) {
        os << tsid.id;
        if (!tsid.tag.empty()) {
            os << "(tag=" << tsid.tag << ')';
        }
        return os;
    }

    // Statefully map ServerId to SocketId.
    class ServerId2SocketIdMapper {
    public:
        ServerId2SocketIdMapper();

        ~ServerId2SocketIdMapper();

        // Remember duplicated count of server.id
        // Returns true if server.id does not exist before.
        bool AddServer(const ServerId &server);

        // Remove 1 duplication of server.id.
        // Returns true if server.id does not exist after.
        bool RemoveServer(const ServerId &server);

        // Remember duplicated counts of all SocketId in servers.
        // Returns list of SocketId that do not exist before.
        std::vector<SocketId> &AddServers(const std::vector<ServerId> &servers);

        // Remove 1 duplication of all SocketId in servers respectively.
        // Returns list of SocketId that do not exist after.
        std::vector<SocketId> &RemoveServers(const std::vector<ServerId> &servers);

    private:
        mutil::FlatMap<SocketId, int> _nref_map;
        std::vector<SocketId> _tmp;
    };

} // namespace melon


namespace MUTIL_HASH_NAMESPACE {
#if defined(COMPILER_GCC)

    template<>
    struct hash<melon::ServerId> {
        std::size_t operator()(const ::melon::ServerId &tagged_id) const {
            return hash<std::string>()(tagged_id.tag) * 101 + tagged_id.id;
        }
    };

#elif defined(COMPILER_MSVC)
    inline size_t hash_value(const ::melon::ServerId& tagged_id) {
        return hash_value(tagged_id.tag) * 101 + tagged_id.id;
    }
#endif  // COMPILER
}

#endif  // MELON_RPC_SERVER_ID_H_
