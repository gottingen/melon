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


#include <melon/rpc/uri.h>
#include <melon/rpc/rtmp/rtmp.h>

#define kMinInputLength 5
#define kMaxInputLength 1024

extern "C" int
LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (size < kMinInputLength || size > kMaxInputLength){
        return 1;
    }

    char *data_in = (char *)malloc(size + 1);
    memcpy(data_in, data, size);
    data_in[size] = '\0';


    std::string input(reinterpret_cast<const char*>(data), size);
    {
        melon::URI uri;
        uri.SetHttpURL(input);
    }
    {
        mutil::StringPiece host;
        mutil::StringPiece vhost;
        mutil::StringPiece port;
        mutil::StringPiece app;
        mutil::StringPiece stream_name;

        melon::ParseRtmpURL(input, &host, &vhost, &port, &app, &stream_name);
    }

    return 0;
}
