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


#include <google/protobuf/io/gzip_stream.h>    // GzipXXXStream
#include "melon/utility/logging.h"
#include "melon/compress/gzip_compress.h"
#include "melon/rpc/protocol.h"


namespace melon::compress {

    static void LogError(const google::protobuf::io::GzipOutputStream &gzip) {
        if (gzip.ZlibErrorMessage()) {
            LOG(WARNING) << "Fail to decompress: " << gzip.ZlibErrorMessage();
        } else {
            LOG(WARNING) << "Fail to decompress.";
        }
    }

    static void LogError(const google::protobuf::io::GzipInputStream &gzip) {
        if (gzip.ZlibErrorMessage()) {
            LOG(WARNING) << "Fail to decompress: " << gzip.ZlibErrorMessage();
        } else {
            LOG(WARNING) << "Fail to decompress.";
        }
    }

    bool GzipCompress(const google::protobuf::Message &msg, mutil::IOBuf *buf) {
        mutil::IOBufAsZeroCopyOutputStream wrapper(buf);
        google::protobuf::io::GzipOutputStream::Options gzip_opt;
        gzip_opt.format = google::protobuf::io::GzipOutputStream::GZIP;
        google::protobuf::io::GzipOutputStream gzip(&wrapper, gzip_opt);
        if (!msg.SerializeToZeroCopyStream(&gzip)) {
            LogError(gzip);
            return false;
        }
        return gzip.Close();
    }

    bool GzipDecompress(const mutil::IOBuf &data, google::protobuf::Message *msg) {
        mutil::IOBufAsZeroCopyInputStream wrapper(data);
        google::protobuf::io::GzipInputStream gzip(
                &wrapper, google::protobuf::io::GzipInputStream::GZIP);
        if (!ParsePbFromZeroCopyStream(msg, &gzip)) {
            LogError(gzip);
            return false;
        }
        return true;
    }

    bool GzipCompress(const mutil::IOBuf &msg, mutil::IOBuf *buf,
                      const GzipCompressOptions *options_in) {
        mutil::IOBufAsZeroCopyOutputStream wrapper(buf);
        google::protobuf::io::GzipOutputStream::Options gzip_opt;
        if (options_in) {
            gzip_opt = *options_in;
        }
        google::protobuf::io::GzipOutputStream out(&wrapper, gzip_opt);
        mutil::IOBufAsZeroCopyInputStream in(msg);
        const void *data_in = NULL;
        int size_in = 0;
        void *data_out = NULL;
        int size_out = 0;
        while (1) {
            if (size_out == 0 && !out.Next(&data_out, &size_out)) {
                break;
            }
            if (size_in == 0 && !in.Next(&data_in, &size_in)) {
                break;
            }
            const int size_cp = std::min(size_in, size_out);
            memcpy(data_out, data_in, size_cp);
            size_in -= size_cp;
            data_in = (char *) data_in + size_cp;
            size_out -= size_cp;
            data_out = (char *) data_out + size_cp;
        }
        if (size_in != 0 || (size_t) in.ByteCount() != msg.size()) {
            // If any stage is not fully consumed, something went wrong.
            LogError(out);
            return false;
        }
        if (size_out != 0) {
            out.BackUp(size_out);
        }
        return out.Close();
    }

    inline bool GzipDecompressBase(
            const mutil::IOBuf &data, mutil::IOBuf *msg,
            google::protobuf::io::GzipInputStream::Format format) {
        mutil::IOBufAsZeroCopyInputStream wrapper(data);
        google::protobuf::io::GzipInputStream in(&wrapper, format);
        mutil::IOBufAsZeroCopyOutputStream out(msg);
        const void *data_in = NULL;
        int size_in = 0;
        void *data_out = NULL;
        int size_out = 0;
        while (1) {
            if (size_out == 0 && !out.Next(&data_out, &size_out)) {
                break;
            }
            if (size_in == 0 && !in.Next(&data_in, &size_in)) {
                break;
            }
            const int size_cp = std::min(size_in, size_out);
            memcpy(data_out, data_in, size_cp);
            size_in -= size_cp;
            data_in = (char *) data_in + size_cp;
            size_out -= size_cp;
            data_out = (char *) data_out + size_cp;
        }
        if (size_in != 0 ||
            (size_t) wrapper.ByteCount() != data.size() ||
            in.Next(&data_in, &size_in)) {
            // If any stage is not fully consumed, something went wrong.
            // Here we call in.Next addtitionally to make sure that the gzip
            // "blackbox" does not have buffer left.
            LogError(in);
            return false;
        }
        if (size_out != 0) {
            out.BackUp(size_out);
        }
        return true;
    }

    bool ZlibCompress(const google::protobuf::Message &res, mutil::IOBuf *buf) {
        mutil::IOBufAsZeroCopyOutputStream wrapper(buf);
        google::protobuf::io::GzipOutputStream::Options zlib_opt;
        zlib_opt.format = google::protobuf::io::GzipOutputStream::ZLIB;
        google::protobuf::io::GzipOutputStream zlib(&wrapper, zlib_opt);
        return res.SerializeToZeroCopyStream(&zlib) && zlib.Close();
    }

    bool ZlibDecompress(const mutil::IOBuf &data, google::protobuf::Message *req) {
        mutil::IOBufAsZeroCopyInputStream wrapper(data);
        google::protobuf::io::GzipInputStream zlib(
                &wrapper, google::protobuf::io::GzipInputStream::ZLIB);
        return ParsePbFromZeroCopyStream(req, &zlib);
    }

    bool GzipDecompress(const mutil::IOBuf &data, mutil::IOBuf *msg) {
        return GzipDecompressBase(
                data, msg, google::protobuf::io::GzipInputStream::GZIP);
    }

    bool ZlibDecompress(const mutil::IOBuf &data, mutil::IOBuf *msg) {
        return GzipDecompressBase(
                data, msg, google::protobuf::io::GzipInputStream::ZLIB);
    }

} // namespace melon::compress
