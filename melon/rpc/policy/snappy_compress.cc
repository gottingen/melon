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


#include "melon/log/logging.h"
#include "melon/io/snappy/snappy.h"
#include "melon/rpc/policy/snappy_compress.h"
#include "melon/rpc/protocol.h"


namespace melon::rpc {
    namespace policy {

        bool SnappyCompress(const google::protobuf::Message &res, melon::cord_buf *buf) {
            melon::cord_buf serialized_pb;
            melon::cord_buf_as_zero_copy_output_stream wrapper(&serialized_pb);
            if (res.SerializeToZeroCopyStream(&wrapper)) {
                melon::cord_buf_as_snappy_source source(serialized_pb);
                melon::cord_buf_as_snappy_sink sink(*buf);
                return melon::snappy::Compress(&source, &sink);
            }
            MELON_LOG(WARNING) << "Fail to serialize input pb=" << &res;
            return false;
        }

        bool SnappyDecompress(const melon::cord_buf &data, google::protobuf::Message *req) {
            melon::cord_buf_as_snappy_source source(data);
            melon::cord_buf binary_pb;
            melon::cord_buf_as_snappy_sink sink(binary_pb);
            if (melon::snappy::Uncompress(&source, &sink)) {
                return ParsePbFromCordBuf(req, binary_pb);
            }
            MELON_LOG(WARNING) << "Fail to snappy::Uncompress, size=" << data.size();
            return false;
        }

        bool SnappyCompress(const melon::cord_buf &in, melon::cord_buf *out) {
            melon::cord_buf_as_snappy_source source(in);
            melon::cord_buf_as_snappy_sink sink(*out);
            return melon::snappy::Compress(&source, &sink);
        }

        bool SnappyDecompress(const melon::cord_buf &in, melon::cord_buf *out) {
            melon::cord_buf_as_snappy_source source(in);
            melon::cord_buf_as_snappy_sink sink(*out);
            return melon::snappy::Uncompress(&source, &sink);
        }

    }  // namespace policy
} // namespace melon::rpc
