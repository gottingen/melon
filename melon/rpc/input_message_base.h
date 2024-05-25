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



#ifndef MELON_RPC_INPUT_MESSAGE_BASE_H_
#define MELON_RPC_INPUT_MESSAGE_BASE_H_

#include <melon/rpc/socket_id.h>           // SocketId
#include <melon/rpc/destroyable.h>         // DestroyingPtr


namespace melon {

// Messages returned by Parse handlers must extend this class
class InputMessageBase : public Destroyable {
protected:
    // Implement this method to customize deletion of this message.
    virtual void DestroyImpl() = 0;
    
public:
    // Called to release the memory of this message instead of "delete"
    void Destroy();
    
    // Own the socket where this message is from.
    Socket* ReleaseSocket();

    // Get the socket where this message is from.
    Socket* socket() const { return _socket.get(); }

    // Arg of the InputMessageHandler which parses this message successfully.
    const void* arg() const { return _arg; }

    // [Internal]
    int64_t received_us() const { return _received_us; }
    int64_t base_real_us() const { return _base_real_us; }

protected:
    virtual ~InputMessageBase();

private:
friend class InputMessenger;
friend void* ProcessInputMessage(void*);
friend class Stream;
    int64_t _received_us;
    int64_t _base_real_us;
    SocketUniquePtr _socket;
    void (*_process)(InputMessageBase* msg);
    const void* _arg;
};

} // namespace melon


#endif  // MELON_RPC_INPUT_MESSAGE_BASE_H_
