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



#ifndef MELON_RPC_REDIS_REDIS_COMMAND_H_
#define MELON_RPC_REDIS_REDIS_COMMAND_H_

#include <limits>
#include <memory>           // std::unique_ptr
#include <vector>
#include <melon/base/iobuf.h>
#include <melon/utility/status.h>
#include <melon/base/arena.h>
#include <melon/rpc/parse_result.h>

namespace melon {

    // Format a redis command and append it to `buf'.
    // Returns mutil::Status::OK() on success.
    mutil::Status RedisCommandFormat(mutil::IOBuf *buf, const char *fmt, ...);

    mutil::Status RedisCommandFormatV(mutil::IOBuf *buf, const char *fmt, va_list args);

    // Just convert the command to the text format of redis without processing the
    // specifiers(%) inside.
    mutil::Status RedisCommandNoFormat(mutil::IOBuf *buf, const mutil::StringPiece &command);

    // Concatenate components to form a redis command.
    mutil::Status RedisCommandByComponents(mutil::IOBuf *buf,
                                           const mutil::StringPiece *components,
                                           size_t num_components);

    // A parser used to parse redis raw command.
    class RedisCommandParser {
    public:
        RedisCommandParser();

        // Parse raw message from `buf'. Return PARSE_OK and set the parsed command
        // to `args' and length to `len' if successful. Memory of args are allocated
        // in `arena'.
        ParseError Consume(mutil::IOBuf &buf, std::vector<mutil::StringPiece> *args,
                           mutil::Arena *arena);

        size_t ParsedArgsSize();

    private:
        // Reset parser to the initial state.
        void Reset();

        bool _parsing_array;            // if the parser has met array indicator '*'
        int _length;                    // array length
        int _index;                     // current parsing array index
        std::vector<mutil::StringPiece> _args;  // parsed command string
    };

} // namespace melon


#endif  // MELON_RPC_REDIS_REDIS_COMMAND_H_
