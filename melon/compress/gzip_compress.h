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


#ifndef MELON_COMPRESS_GZIP_COMPRESS_H_
#define MELON_COMPRESS_GZIP_COMPRESS_H_

#include <google/protobuf/message.h>              // Message
#include <google/protobuf/io/gzip_stream.h>
#include "melon/utility/iobuf.h"                           // mutil::IOBuf


namespace melon::compress {

    typedef google::protobuf::io::GzipOutputStream::Options GzipCompressOptions;

    // Compress serialized `msg' into `buf'.
    bool GzipCompress(const google::protobuf::Message &msg, mutil::IOBuf *buf);

    bool ZlibCompress(const google::protobuf::Message &msg, mutil::IOBuf *buf);

    // Parse `msg' from decompressed `buf'.
    bool GzipDecompress(const mutil::IOBuf &buf, google::protobuf::Message *msg);

    bool ZlibDecompress(const mutil::IOBuf &buf, google::protobuf::Message *msg);

    // Put compressed `in' into `out'.
    bool GzipCompress(const mutil::IOBuf &in, mutil::IOBuf *out,
                      const GzipCompressOptions *);

    // Put decompressed `in' into `out'.
    bool GzipDecompress(const mutil::IOBuf &in, mutil::IOBuf *out);

    bool ZlibDecompress(const mutil::IOBuf &in, mutil::IOBuf *out);

} // namespace melon::compress


#endif // MELON_COMPRESS_GZIP_COMPRESS_H_
