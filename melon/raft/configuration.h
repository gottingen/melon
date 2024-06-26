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

#include <string>
#include <ostream>
#include <vector>
#include <set>
#include <map>
#include <melon/utility/strings/string_piece.h>
#include <melon/utility/endpoint.h>
#include <turbo/log/logging.h>

namespace melon::raft {

    typedef std::string GroupId;
    // GroupId with version, format: {group_id}_{index}
    typedef std::string VersionedGroupId;

    enum Role {
        REPLICA = 0,
        WITNESS = 1,
    };

// Represent a participant in a replicating group.
    struct PeerId {
        mutil::EndPoint addr; // ip+port.
        int idx; // idx in same addr, default 0
        Role role = REPLICA;

        PeerId() : idx(0), role(REPLICA) {}

        explicit PeerId(mutil::EndPoint addr_) : addr(addr_), idx(0), role(REPLICA) {}

        PeerId(mutil::EndPoint addr_, int idx_) : addr(addr_), idx(idx_), role(REPLICA) {}

        PeerId(mutil::EndPoint addr_, int idx_, bool witness) : addr(addr_), idx(idx_) {
            if (witness) {
                this->role = WITNESS;
            }
        }

        /*intended implicit*/PeerId(const std::string &str) { CHECK_EQ(0, parse(str)); }

        PeerId(const PeerId &id) : addr(id.addr), idx(id.idx), role(id.role) {}

        void reset() {
            addr.ip = mutil::IP_ANY;
            addr.port = 0;
            idx = 0;
            role = REPLICA;
        }

        bool is_empty() const {
            return (addr.ip == mutil::IP_ANY && addr.port == 0 && idx == 0);
        }

        bool is_witness() const {
            return role == WITNESS;
        }

        int parse(const std::string &str) {
            reset();
            char ip_str[64];
            int value = REPLICA;
            if (2 > sscanf(str.c_str(), "%[^:]%*[:]%d%*[:]%d%*[:]%d", ip_str, &addr.port, &idx, &value)) {
                reset();
                return -1;
            }
            role = (Role) value;
            if (role > WITNESS) {
                reset();
                return -1;
            }
            if (0 != mutil::str2ip(ip_str, &addr.ip)) {
                reset();
                return -1;
            }
            return 0;
        }

        std::string to_string() const {
            char str[128];
            snprintf(str, sizeof(str), "%s:%d:%d", mutil::endpoint2str(addr).c_str(), idx, int(role));
            return std::string(str);
        }

        PeerId &operator=(const PeerId &rhs) = default;
    };

    inline bool operator<(const PeerId &id1, const PeerId &id2) {
        if (id1.addr < id2.addr) {
            return true;
        } else {
            return id1.addr == id2.addr && id1.idx < id2.idx;
        }
    }

    inline bool operator==(const PeerId &id1, const PeerId &id2) {
        return (id1.addr == id2.addr && id1.idx == id2.idx);
    }

    inline bool operator!=(const PeerId &id1, const PeerId &id2) {
        return !(id1 == id2);
    }

    inline std::ostream &operator<<(std::ostream &os, const PeerId &id) {
        return os << id.addr << ':' << id.idx << ':' << int(id.role);
    }

    struct NodeId {
        GroupId group_id;
        PeerId peer_id;

        NodeId(const GroupId &group_id_, const PeerId &peer_id_)
                : group_id(group_id_), peer_id(peer_id_) {
        }

        std::string to_string() const;
    };

    inline bool operator<(const NodeId &id1, const NodeId &id2) {
        const int rc = id1.group_id.compare(id2.group_id);
        if (rc < 0) {
            return true;
        } else {
            return rc == 0 && id1.peer_id < id2.peer_id;
        }
    }

    inline bool operator==(const NodeId &id1, const NodeId &id2) {
        return (id1.group_id == id2.group_id && id1.peer_id == id2.peer_id);
    }

    inline bool operator!=(const NodeId &id1, const NodeId &id2) {
        return (id1.group_id != id2.group_id || id1.peer_id != id2.peer_id);
    }

    inline std::ostream &operator<<(std::ostream &os, const NodeId &id) {
        return os << id.group_id << ':' << id.peer_id;
    }

    inline std::string NodeId::to_string() const {
        std::ostringstream oss;
        oss << *this;
        return oss.str();
    }

// A set of peers.
    class Configuration {
    public:
        typedef std::set<PeerId>::const_iterator const_iterator;

        // Construct an empty configuration.
        Configuration() {}

        // Construct from peers stored in std::vector.
        explicit Configuration(const std::vector<PeerId> &peers) {
            for (size_t i = 0; i < peers.size(); i++) {
                _peers.insert(peers[i]);
            }
        }

        // Construct from peers stored in std::set
        explicit Configuration(const std::set<PeerId> &peers) : _peers(peers) {}

        // Assign from peers stored in std::vector
        void operator=(const std::vector<PeerId> &peers) {
            _peers.clear();
            for (size_t i = 0; i < peers.size(); i++) {
                _peers.insert(peers[i]);
            }
        }

        // Assign from peers stored in std::set
        void operator=(const std::set<PeerId> &peers) {
            _peers = peers;
        }

        // Remove all peers.
        void reset() { _peers.clear(); }

        bool empty() const { return _peers.empty(); }

        size_t size() const { return _peers.size(); }

        const_iterator begin() const { return _peers.begin(); }

        const_iterator end() const { return _peers.end(); }

        // Clear the container and put peers in.
        void list_peers(std::set<PeerId> *peers) const {
            peers->clear();
            *peers = _peers;
        }

        void list_peers(std::vector<PeerId> *peers) const {
            peers->clear();
            peers->reserve(_peers.size());
            std::set<PeerId>::iterator it;
            for (it = _peers.begin(); it != _peers.end(); ++it) {
                peers->push_back(*it);
            }
        }

        void append_peers(std::set<PeerId> *peers) {
            peers->insert(_peers.begin(), _peers.end());
        }

        // Add a peer.
        // Returns true if the peer is newly added.
        bool add_peer(const PeerId &peer) {
            return _peers.insert(peer).second;
        }

        // Remove a peer.
        // Returns true if the peer is removed.
        bool remove_peer(const PeerId &peer) {
            return _peers.erase(peer);
        }

        // True if the peer exists.
        bool contains(const PeerId &peer_id) const {
            return _peers.find(peer_id) != _peers.end();
        }

        // True if ALL peers exist.
        bool contains(const std::vector<PeerId> &peers) const {
            for (size_t i = 0; i < peers.size(); i++) {
                if (_peers.find(peers[i]) == _peers.end()) {
                    return false;
                }
            }
            return true;
        }

        // True if peers are same.
        bool equals(const std::vector<PeerId> &peers) const {
            std::set<PeerId> peer_set;
            for (size_t i = 0; i < peers.size(); i++) {
                if (_peers.find(peers[i]) == _peers.end()) {
                    return false;
                }
                peer_set.insert(peers[i]);
            }
            return peer_set.size() == _peers.size();
        }

        bool equals(const Configuration &rhs) const {
            if (size() != rhs.size()) {
                return false;
            }
            // The cost of the following routine is O(nlogn), which is not the best
            // approach.
            for (const_iterator iter = begin(); iter != end(); ++iter) {
                if (!rhs.contains(*iter)) {
                    return false;
                }
            }
            return true;
        }

        // Get the difference between |*this| and |rhs|
        // |included| would be assigned to |*this| - |rhs|
        // |excluded| would be assigned to |rhs| - |*this|
        void diffs(const Configuration &rhs,
                   Configuration *included,
                   Configuration *excluded) const {
            *included = *this;
            *excluded = rhs;
            for (std::set<PeerId>::const_iterator
                         iter = _peers.begin(); iter != _peers.end(); ++iter) {
                excluded->_peers.erase(*iter);
            }
            for (std::set<PeerId>::const_iterator
                         iter = rhs._peers.begin(); iter != rhs._peers.end(); ++iter) {
                included->_peers.erase(*iter);
            }
        }

        // Parse Configuration from a string into |this|
        // Returns 0 on success, -1 otherwise
        int parse_from(mutil::StringPiece conf);

    private:
        std::set<PeerId> _peers;

    };

    std::ostream &operator<<(std::ostream &os, const Configuration &a);

}  //  namespace melon::raft
