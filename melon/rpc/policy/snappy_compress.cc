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


#include "turbo/log/logging.h"
#include "turbo/io/snappy/snappy.h"
#include "melon/rpc/policy/snappy_compress.h"
#include "melon/rpc/protocol.h"


namespace melon::rpc {
    namespace policy {

        bool SnappyCompress(const google::protobuf::Message &res, turbo::cord_buf *buf) {
            turbo::cord_buf serialized_pb;
            turbo::cord_buf_as_zero_copy_output_stream wrapper(&serialized_pb);
            if (res.SerializeToZeroCopyStream(&wrapper)) {
                turbo::cord_buf_as_snappy_source source(serialized_pb);
                turbo::cord_buf_as_snappy_sink sink(*buf);
                return turbo::snappy::Compress(&source, &sink);
            }
            TURBO_LOG(WARNING) << "Fail to serialize input pb=" << &res;
            return false;
        }

        bool SnappyDecompress(const turbo::cord_buf &data, google::protobuf::Message *req) {
            turbo::cord_buf_as_snappy_source source(data);
            turbo::cord_buf binary_pb;
            turbo::cord_buf_as_snappy_sink sink(binary_pb);
            if (turbo::snappy::Uncompress(&source, &sink)) {
                return ParsePbFromCordBuf(req, binary_pb);
            }
            TURBO_LOG(WARNING) << "Fail to snappy::Uncompress, size=" << data.size();
            return false;
        }

        bool SnappyCompress(const turbo::cord_buf &in, turbo::cord_buf *out) {
            turbo::cord_buf_as_snappy_source source(in);
            turbo::cord_buf_as_snappy_sink sink(*out);
            return turbo::snappy::Compress(&source, &sink);
        }

        bool SnappyDecompress(const turbo::cord_buf &in, turbo::cord_buf *out) {
            turbo::cord_buf_as_snappy_source source(in);
            turbo::cord_buf_as_snappy_sink sink(*out);
            return turbo::snappy::Uncompress(&source, &sink);
        }

    }  // namespace policy
} // namespace melon::rpc
