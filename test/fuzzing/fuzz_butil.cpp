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


#include "melon/utility/base64.h"
#include "melon/utility/crc32c.h"
#include "melon/utility/hash.h"
#include "melon/utility/sha1.h"

#define kMinInputLength 5
#define kMaxInputLength 1024

extern "C" int
LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (size < kMinInputLength || size > kMaxInputLength){
        return 1;
    }

    std::string input(reinterpret_cast<const char*>(data), size);

    {
        std::string encoded;
        std::string decoded;
        mutil::Base64Encode(input, &encoded);
        mutil::Base64Decode(input, &decoded);
    }
    {
        mutil::crc32c::Value(reinterpret_cast<const char*>(data), size);
    }
    {
        mutil::Hash(input);
    }
    {
        mutil::SHA1HashString(input);
    }

    return 0;
}
