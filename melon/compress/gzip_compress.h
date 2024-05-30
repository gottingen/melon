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

#pragma once

#include <google/protobuf/message.h>              // Message
#include <google/protobuf/io/gzip_stream.h>
#include <melon/utility/iobuf.h>                           // mutil::IOBuf


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
