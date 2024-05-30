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



#ifndef MELON_RPC_STREAM_CREATOR_H_
#define MELON_RPC_STREAM_CREATOR_H_

#include <melon/rpc/socket_id.h>

namespace melon {
class Controller;
class StreamUserData;

// Abstract creation of "user-level connection" over a RPC-like process.
// Lifetime of this object should be guaranteed by user during the RPC,
// generally this object is created before RPC and destroyed after RPC.
class StreamCreator {
public:
    virtual ~StreamCreator() = default;

    // Called when the socket for sending request is about to be created.
    // If the RPC has retries, this function MAY be called before each retry.
    // This function would not be called if some preconditions are not
    // satisfied.
    // Params:
    //  inout: pointing to the socket to send requests by default,
    //    replaceable by user created ones (or keep as it is). remote_side()
    //    of the replaced socket must be same with the default socket.
    //    The replaced socket should take cntl->connection_type() into account
    //    since the framework sends request by the replaced socket directly
    //    when stream_creator is present.
    //  cntl: contains contexts of the RPC, if there's any error during
    //    replacement, call cntl->SetFailed().
    virtual StreamUserData* OnCreatingStream(SocketUniquePtr* inout,
                                             Controller* cntl) = 0;

    // Called when the StreamCreator is about to destroyed.
    // This function MUST be called only once at the end of successful RPC
    // Call to recycle resources.
    // Params:
    //   cntl: contexts of the RPC
    virtual void DestroyStreamCreator(Controller* cntl) = 0;
};

// The Intermediate user data created by StreamCreator to record the context
// of a specific stream request.
class StreamUserData {
public:
    virtual ~StreamUserData() = default;

    // Called when the streamUserData is about to destroyed.
    // This function MUST be called to clean up resources if OnCreatingStream
    // of StreamCreator has returned a valid StreamUserData pointer.
    // Params:
    //   sending_sock: The socket chosen by OnCreatingStream(), if an error
    //     happens during choosing, the enclosed socket is NULL.
    //   cntl: contexts of the RPC.
    //   error_code: Use this instead of cntl->ErrorCode().
    //   end_of_rpc: true if the RPC is about to destroyed.
    virtual void DestroyStreamUserData(SocketUniquePtr& sending_sock,
                                       Controller* cntl,
                                       int error_code,
                                       bool end_of_rpc) = 0;
};

} // namespace melon


#endif  // MELON_RPC_STREAM_CREATOR_H_
