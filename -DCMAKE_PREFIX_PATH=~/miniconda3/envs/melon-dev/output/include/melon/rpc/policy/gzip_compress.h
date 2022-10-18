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


#ifndef MELON_RPC_POLICY_GZIP_COMPRESS_H_
#define MELON_RPC_POLICY_GZIP_COMPRESS_H_

#include <google/protobuf/message.h>              // Message
#include <google/protobuf/io/gzip_stream.h>
#include "melon/io/cord_buf.h"                           // melon::cord_buf


namespace melon::rpc {
    namespace policy {

        typedef google::protobuf::io::GzipOutputStream::Options GzipCompressOptions;

        // Compress serialized `msg' into `buf'.
        bool GzipCompress(const google::protobuf::Message &msg, melon::cord_buf *buf);

        bool ZlibCompress(const google::protobuf::Message &msg, melon::cord_buf *buf);

        // Parse `msg' from decompressed `buf'.
        bool GzipDecompress(const melon::cord_buf &buf, google::protobuf::Message *msg);

        bool ZlibDecompress(const melon::cord_buf &buf, google::protobuf::Message *msg);

        // Put compressed `in' into `out'.
        bool GzipCompress(const melon::cord_buf &in, melon::cord_buf *out,
                                  const GzipCompressOptions *);

        // Put decompressed `in' into `out'.
        bool GzipDecompress(const melon::cord_buf &in, melon::cord_buf *out);

        bool ZlibDecompress(const melon::cord_buf &in, melon::cord_buf *out);

    }  // namespace policy
} // namespace melon::rpc


#endif // MELON_RPC_POLICY_GZIP_COMPRESS_H_
