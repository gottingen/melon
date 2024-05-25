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

#include <melon/raft/configuration.h>
#include <melon/utility/logging.h>
#include <melon/utility/string_splitter.h>

namespace melon::raft {

    std::ostream &operator<<(std::ostream &os, const Configuration &a) {
        std::vector<PeerId> peers;
        a.list_peers(&peers);
        for (size_t i = 0; i < peers.size(); i++) {
            os << peers[i];
            if (i < peers.size() - 1) {
                os << ",";
            }
        }
        return os;
    }

    int Configuration::parse_from(mutil::StringPiece conf) {
        reset();
        std::string peer_str;
        for (mutil::StringSplitter sp(conf.begin(), conf.end(), ','); sp; ++sp) {
            melon::raft::PeerId peer;
            peer_str.assign(sp.field(), sp.length());
            if (peer.parse(peer_str) != 0) {
                MLOG(ERROR) << "Fail to parse " << peer_str;
                return -1;
            }
            add_peer(peer);
        }
        return 0;
    }

}
