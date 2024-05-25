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


#include <google/protobuf/io/gzip_stream.h>    // GzipXXXStream
#include <melon/utility/logging.h>
#include <melon/compress/gzip_compress.h>
#include <melon/rpc/protocol.h>


namespace melon::compress {

    static void LogError(const google::protobuf::io::GzipOutputStream &gzip) {
        if (gzip.ZlibErrorMessage()) {
            MLOG(WARNING) << "Fail to decompress: " << gzip.ZlibErrorMessage();
        } else {
            MLOG(WARNING) << "Fail to decompress.";
        }
    }

    static void LogError(const google::protobuf::io::GzipInputStream &gzip) {
        if (gzip.ZlibErrorMessage()) {
            MLOG(WARNING) << "Fail to decompress: " << gzip.ZlibErrorMessage();
        } else {
            MLOG(WARNING) << "Fail to decompress.";
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
