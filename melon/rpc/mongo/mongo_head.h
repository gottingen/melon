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


#ifndef MELON_RPC_MONGO_MONGO_HEAD_H_
#define MELON_RPC_MONGO_MONGO_HEAD_H_

#include <turbo/base/endian.h>

namespace melon {

        // Sync with
        //   https://github.com/mongodb/mongo-c-driver/blob/master/src/mongoc/mongoc-opcode.h
        //   https://docs.mongodb.org/manual/reference/mongodb-wire-protocol/#request-opcodes
    enum MongoOpCode {
        MONGO_OPCODE_REPLY = 1,
        MONGO_OPCODE_MSG = 1000,
        MONGO_OPCODE_UPDATE = 2001,
        MONGO_OPCODE_INSERT = 2002,
        MONGO_OPCODE_QUERY = 2004,
        MONGO_OPCODE_GET_MORE = 2005,
        MONGO_OPCODE_DELETE = 2006,
        MONGO_OPCODE_KILL_CURSORS = 2007,
    };

    inline bool is_mongo_opcode(int32_t op_code) {
        switch (op_code) {
            case MONGO_OPCODE_REPLY:
                return true;
            case MONGO_OPCODE_MSG:
                return true;
            case MONGO_OPCODE_UPDATE:
                return true;
            case MONGO_OPCODE_INSERT:
                return true;
            case MONGO_OPCODE_QUERY:
                return true;
            case MONGO_OPCODE_GET_MORE:
                return true;
            case MONGO_OPCODE_DELETE:
                return true;
            case MONGO_OPCODE_KILL_CURSORS :
                return true;
        }
        return false;
    }

// All data of mongo protocol is little-endian.
// https://docs.mongodb.org/manual/reference/mongodb-wire-protocol/#byte-ordering
#pragma pack(1)

    struct mongo_head_t {
        int32_t message_length;  // total message size, including this
        int32_t request_id;      // identifier for this message
        int32_t response_to;     // requestID from the original request
        // (used in responses from db)
        int32_t op_code;         // request type, see MongoOpCode.

        void make_host_endian() {
            if (!ARCH_CPU_LITTLE_ENDIAN) {
                message_length = turbo::gbswap_32((uint32_t) message_length);
                request_id =turbo::gbswap_32((uint32_t) request_id);
                response_to = turbo::gbswap_32((uint32_t) response_to);
                op_code = turbo::gbswap_32((uint32_t) op_code);
            }
        }
    };

#pragma pack()

} // namespace melon


#endif // MELON_RPC_MONGO_MONGO_HEAD_H_
