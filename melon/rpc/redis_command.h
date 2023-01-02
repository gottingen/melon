// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.


#ifndef MELON_RPC_REDIS_COMMAND_H_
#define MELON_RPC_REDIS_COMMAND_H_

#include <memory>           // std::unique_ptr
#include <vector>
#include <limits>
#include "melon/io/cord_buf.h"
#include "melon/base/result_status.h"
#include "melon/memory/arena.h"
#include "melon/rpc/parse_result.h"

namespace melon::rpc {

    // Format a redis command and append it to `buf'.
    // Returns melon::result_status::success() on success.
    melon::result_status RedisCommandFormat(melon::cord_buf *buf, const char *fmt, ...);

    melon::result_status RedisCommandFormatV(melon::cord_buf *buf, const char *fmt, va_list args);

    // Just convert the command to the text format of redis without processing the
    // specifiers(%) inside.
    melon::result_status RedisCommandNoFormat(melon::cord_buf *buf, const std::string_view &command);

    // Concatenate components to form a redis command.
    melon::result_status RedisCommandByComponents(melon::cord_buf *buf,
                                                       const std::string_view *components,
                                                       size_t num_components);

    // A parser used to parse redis raw command.
    class RedisCommandParser {
    public:
        RedisCommandParser();

        // Parse raw message from `buf'. Return PARSE_OK and set the parsed command
        // to `args' and length to `len' if successful. Memory of args are allocated
        // in `arena'.
        ParseError Consume(melon::cord_buf &buf, std::vector<std::string_view> *args,
                           melon::Arena *arena);

    private:
        // Reset parser to the initial state.
        void Reset();

        bool _parsing_array;            // if the parser has met array indicator '*'
        int _length;                    // array length
        int _index;                     // current parsing array index
        std::vector<std::string_view> _args;  // parsed command string
    };

} // namespace melon::rpc


#endif  // MELON_RPC_REDIS_COMMAND_H_
