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


#include <melon/rpc/redis/redis.h>
#include <melon/rpc/redis/redis_command.h>

#define kMinInputLength 5
#define kMaxInputLength 1024

extern "C" int
LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (size < kMinInputLength || size > kMaxInputLength){
        return 1;
    }

    std::string input(reinterpret_cast<const char*>(data), size);
    mutil::IOBuf buf;
    buf.append(input);
    {
        mutil::Arena arena;
        melon::RedisCommandParser parser;
        std::vector<mutil::StringPiece> command_out;
        parser.Consume(buf, &command_out, &arena);
    }
    {
        mutil::Arena arena;
        melon::RedisReply r2(&arena);
        r2.ConsumePartialIOBuf(buf);
    }

    return 0;
}

