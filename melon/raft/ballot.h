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

#include <melon/raft/configuration.h>

namespace melon::raft {

    class Ballot {
    public:
        struct PosHint {
            PosHint() : pos0(-1), pos1(-1) {}

            int pos0;
            int pos1;
        };

        Ballot();

        ~Ballot();

        void swap(Ballot &rhs) {
            _peers.swap(rhs._peers);
            std::swap(_quorum, rhs._quorum);
            _old_peers.swap(rhs._old_peers);
            std::swap(_old_quorum, rhs._old_quorum);
        }

        int init(const Configuration &conf, const Configuration *old_conf);

        PosHint grant(const PeerId &peer, PosHint hint);

        void grant(const PeerId &peer);

        bool granted() const { return _quorum <= 0 && _old_quorum <= 0; }

    private:
        struct UnfoundPeerId {
            UnfoundPeerId(const PeerId &peer_id) : peer_id(peer_id), found(false) {}

            PeerId peer_id;
            bool found;

            bool operator==(const PeerId &id) const {
                return peer_id == id;
            }
        };

        std::vector<UnfoundPeerId>::iterator find_peer(
                const PeerId &peer, std::vector<UnfoundPeerId> &peers, int pos_hint) {
            if (pos_hint < 0 || pos_hint >= (int) peers.size()
                || peers[pos_hint].peer_id != peer) {
                for (std::vector<UnfoundPeerId>::iterator
                             iter = peers.begin(); iter != peers.end(); ++iter) {
                    if (*iter == peer) {
                        return iter;
                    }
                }
                return peers.end();
            }
            return peers.begin() + pos_hint;
        }

        std::vector<UnfoundPeerId> _peers;
        int _quorum;
        std::vector<UnfoundPeerId> _old_peers;
        int _old_quorum;
    };

};