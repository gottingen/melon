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

#include <melon/raft/configuration.h>
#include <turbo/log/logging.h>
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

    int Configuration::parse_from(std::string_view conf) {
        reset();
        std::string peer_str;
        for (mutil::StringSplitter sp(conf.begin(), conf.end(), ','); sp; ++sp) {
            melon::raft::PeerId peer;
            peer_str.assign(sp.field(), sp.length());
            if (peer.parse(peer_str) != 0) {
                LOG(ERROR) << "Fail to parse " << peer_str;
                return -1;
            }
            add_peer(peer);
        }
        return 0;
    }

}
