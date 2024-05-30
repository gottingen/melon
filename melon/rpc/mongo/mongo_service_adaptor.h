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


#ifndef MELON_RPC_MONGO_MONGO_SERVICE_ADAPTOR_H_
#define MELON_RPC_MONGO_MONGO_SERVICE_ADAPTOR_H_

#include <melon/utility/iobuf.h>
#include <melon/rpc/input_message_base.h>
#include <melon/rpc/shared_object.h>


namespace melon {

// custom mongo context. derive this and implement your own functionalities.
class MongoContext : public SharedObject {
public:
    virtual ~MongoContext() {}
};

// a container of custom mongo context. created by ParseMongoRequest when the first msg comes over
// a socket. it lives as long as the socket.
class MongoContextMessage : public InputMessageBase {
public:
    MongoContextMessage(MongoContext *context) : _context(context) {}
    // @InputMessageBase
    void DestroyImpl() { delete this; }
    MongoContext* context() { return _context.get(); }

private:
    mutil::intrusive_ptr<MongoContext> _context;
};

class MongoServiceAdaptor {
public:
    // Make an error msg when the cntl fails. If cntl fails, we must send mongo client a msg not 
    // only to indicate the error, but also to finish the round trip.
    virtual void SerializeError(int response_to, mutil::IOBuf* out_buf) const = 0;

    // Create a custom context which is attached to socket. This func is called only when the first
    // msg from the socket comes. The context will be destroyed when the socket is closed.
    virtual MongoContext* CreateSocketContext() const = 0;
};

} // namespace melon


#endif  // MELON_RPC_MONGO_MONGO_SERVICE_ADAPTOR_H_

