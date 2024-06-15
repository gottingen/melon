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


#ifndef MELON_RPC_SERVER_NODE_H_
#define MELON_RPC_SERVER_NODE_H_

#include <string>
#include <melon/base/endpoint.h>

namespace melon {

// Representing a server inside a NamingService.
struct ServerNode {
    ServerNode()  = default;
    
    explicit ServerNode(const mutil::EndPoint& pt) : addr(pt) {}

    ServerNode(mutil::ip_t ip, int port, const std::string& tag2)
        : addr(ip, port), tag(tag2) {}

    ServerNode(const mutil::EndPoint& pt, const std::string& tag2)
        : addr(pt), tag(tag2) {}

    ServerNode(mutil::ip_t ip, int port) : addr(ip, port) {}

    mutil::EndPoint addr;
    std::string tag;
};

inline bool operator<(const ServerNode& n1, const ServerNode& n2)
{ return n1.addr != n2.addr ? (n1.addr < n2.addr) : (n1.tag < n2.tag); }

inline bool operator==(const ServerNode& n1, const ServerNode& n2)
{ return n1.addr == n2.addr && n1.tag == n2.tag; }

inline bool operator!=(const ServerNode& n1, const ServerNode& n2)
{ return !(n1 == n2); }

inline std::ostream& operator<<(std::ostream& os, const ServerNode& n) {
    os << n.addr;
    if (!n.tag.empty()) {
        os << "(tag=" << n.tag << ')';
    }
    return os;
}

} // namespace melon

#endif  // MELON_RPC_SERVER_NODE_H_
