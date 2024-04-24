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


#ifndef  MELON_RAFT_BALLOT_H_
#define  MELON_RAFT_BALLOT_H_

#include "melon/raft/configuration.h"

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

#endif  // MELON_RAFT_BALLOT_H_
