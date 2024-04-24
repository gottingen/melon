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

#ifndef MELON_COMPRESS_SNAPPY_COMPRESS_H_
#define MELON_COMPRESS_SNAPPY_COMPRESS_H_

#include <google/protobuf/message.h>          // Message
#include "melon/utility/iobuf.h"                       // IOBuf


namespace melon::compress {

    // Compress serialized `msg' into `buf'.
    bool SnappyCompress(const google::protobuf::Message &msg, mutil::IOBuf *buf);

    // Parse `msg' from decompressed `buf'
    bool SnappyDecompress(const mutil::IOBuf &data, google::protobuf::Message *msg);

    // Put compressed `in' into `out'.
    bool SnappyCompress(const mutil::IOBuf &in, mutil::IOBuf *out);

    // Put decompressed `in' into `out'.
    bool SnappyDecompress(const mutil::IOBuf &in, mutil::IOBuf *out);

} // namespace melon::compress


#endif // MELON_COMPRESS_SNAPPY_COMPRESS_H_
