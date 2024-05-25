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



#include <melon/utility/logging.h>
#include <melon/rpc/compress.h>
#include <melon/rpc/protocol.h>


namespace melon {

    static const int MAX_HANDLER_SIZE = 1024;
    static CompressHandler s_handler_map[MAX_HANDLER_SIZE] = {{NULL, NULL, NULL}};

    int RegisterCompressHandler(CompressType type,
                                CompressHandler handler) {
        if (NULL == handler.Compress || NULL == handler.Decompress) {
            MLOG(FATAL) << "Invalid parameter: handler function is NULL";
            return -1;
        }
        int index = type;
        if (index < 0 || index >= MAX_HANDLER_SIZE) {
            MLOG(FATAL) << "CompressType=" << type << " is out of range";
            return -1;
        }
        if (s_handler_map[index].Compress != NULL) {
            MLOG(FATAL) << "CompressType=" << type << " was registered";
            return -1;
        }
        s_handler_map[index] = handler;
        return 0;
    }

    // Find CompressHandler by type.
    // Returns NULL if not found
    inline const CompressHandler *FindCompressHandler(CompressType type) {
        int index = type;
        if (index < 0 || index >= MAX_HANDLER_SIZE) {
            MLOG(ERROR) << "CompressType=" << type << " is out of range";
            return NULL;
        }
        if (NULL == s_handler_map[index].Compress) {
            return NULL;
        }
        return &s_handler_map[index];
    }

    const char *CompressTypeToCStr(CompressType type) {
        if (type == COMPRESS_TYPE_NONE) {
            return "none";
        }
        const CompressHandler *handler = FindCompressHandler(type);
        return (handler != NULL ? handler->name : "unknown");
    }

    void ListCompressHandler(std::vector<CompressHandler> *vec) {
        vec->clear();
        for (int i = 0; i < MAX_HANDLER_SIZE; ++i) {
            if (s_handler_map[i].Compress != NULL) {
                vec->push_back(s_handler_map[i]);
            }
        }
    }

    bool ParseFromCompressedData(const mutil::IOBuf &data,
                                 google::protobuf::Message *msg,
                                 CompressType compress_type) {
        if (compress_type == COMPRESS_TYPE_NONE) {
            return ParsePbFromIOBuf(msg, data);
        }
        const CompressHandler *handler = FindCompressHandler(compress_type);
        if (NULL != handler) {
            return handler->Decompress(data, msg);
        }
        return false;
    }

    bool SerializeAsCompressedData(const google::protobuf::Message &msg,
                                   mutil::IOBuf *buf, CompressType compress_type) {
        if (compress_type == COMPRESS_TYPE_NONE) {
            mutil::IOBufAsZeroCopyOutputStream wrapper(buf);
            return msg.SerializeToZeroCopyStream(&wrapper);
        }
        const CompressHandler *handler = FindCompressHandler(compress_type);
        if (NULL != handler) {
            return handler->Compress(msg, buf);
        }
        return false;
    }

} // namespace melon
