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



#ifndef MELON_RPC_REDIS_REDIS_COMMAND_H_
#define MELON_RPC_REDIS_REDIS_COMMAND_H_

#include <limits>
#include <memory>           // std::unique_ptr
#include <vector>
#include "melon/utility/iobuf.h"
#include "melon/utility/status.h"
#include "melon/utility/arena.h"
#include "melon/rpc/parse_result.h"

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
