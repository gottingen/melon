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



#ifndef MELON_RPC_SOCKET_ID_H_
#define MELON_RPC_SOCKET_ID_H_



#include <stdint.h>               // uint64_t
#include <melon/utility/unique_ptr.h>      // std::unique_ptr


namespace melon {

// Unique identifier of a Socket.
// Users shall store SocketId instead of Sockets and call Socket::Address()
// to convert the identifier to an unique_ptr at each access. Whenever a
// unique_ptr is not destructed, the enclosed Socket will not be recycled.
typedef uint64_t SocketId;

const SocketId INVALID_SOCKET_ID = (SocketId)-1;

class Socket;

extern void DereferenceSocket(Socket*);

struct SocketDeleter {
    void operator()(Socket* m) const {
        DereferenceSocket(m);
    }
};

typedef std::unique_ptr<Socket, SocketDeleter> SocketUniquePtr;

} // namespace melon


#endif  // MELON_RPC_SOCKET_ID_H_
