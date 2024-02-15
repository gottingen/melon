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


#include "melon/utility/logging.h"
#include "melon/utility/third_party/snappy/snappy.h"
#include "melon/compress/snappy_compress.h"
#include "melon/rpc/protocol.h"


namespace melon::compress {

    bool SnappyCompress(const google::protobuf::Message &res, mutil::IOBuf *buf) {
        mutil::IOBuf serialized_pb;
        mutil::IOBufAsZeroCopyOutputStream wrapper(&serialized_pb);
        if (res.SerializeToZeroCopyStream(&wrapper)) {
            mutil::IOBufAsSnappySource source(serialized_pb);
            mutil::IOBufAsSnappySink sink(*buf);
            return mutil::snappy::Compress(&source, &sink);
        }
        LOG(WARNING) << "Fail to serialize input pb=" << &res;
        return false;
    }

    bool SnappyDecompress(const mutil::IOBuf &data, google::protobuf::Message *req) {
        mutil::IOBufAsSnappySource source(data);
        mutil::IOBuf binary_pb;
        mutil::IOBufAsSnappySink sink(binary_pb);
        if (mutil::snappy::Uncompress(&source, &sink)) {
            return ParsePbFromIOBuf(req, binary_pb);
        }
        LOG(WARNING) << "Fail to snappy::Uncompress, size=" << data.size();
        return false;
    }

    bool SnappyCompress(const mutil::IOBuf &in, mutil::IOBuf *out) {
        mutil::IOBufAsSnappySource source(in);
        mutil::IOBufAsSnappySink sink(*out);
        return mutil::snappy::Compress(&source, &sink);
    }

    bool SnappyDecompress(const mutil::IOBuf &in, mutil::IOBuf *out) {
        mutil::IOBufAsSnappySource source(in);
        mutil::IOBufAsSnappySink sink(*out);
        return mutil::snappy::Uncompress(&source, &sink);
    }

} // namespace melon::compress
