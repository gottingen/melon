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


#include <melon/utility/base64.h>
#include <melon/utility/crc32c.h>
#include <melon/utility/hash.h>
#include <melon/utility/sha1.h>

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
