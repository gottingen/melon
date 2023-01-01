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
#include "melon/rpc/compress.h"
#include "melon/rpc/protocol.h"


namespace melon::rpc {

    static const int MAX_HANDLER_SIZE = 1024;
    static CompressHandler s_handler_map[MAX_HANDLER_SIZE] = {{nullptr, nullptr, nullptr}};

    int RegisterCompressHandler(CompressType type,
                                CompressHandler handler) {
        if (nullptr == handler.Compress || nullptr == handler.Decompress) {
            MELON_LOG(FATAL) << "Invalid parameter: handler function is nullptr";
            return -1;
        }
        int index = type;
        if (index < 0 || index >= MAX_HANDLER_SIZE) {
            MELON_LOG(FATAL) << "CompressType=" << type << " is out of range";
            return -1;
        }
        if (s_handler_map[index].Compress != nullptr) {
            MELON_LOG(FATAL) << "CompressType=" << type << " was registered";
            return -1;
        }
        s_handler_map[index] = handler;
        return 0;
    }

// Find CompressHandler by type.
// Returns nullptr if not found
    inline const CompressHandler *FindCompressHandler(CompressType type) {
        int index = type;
        if (index < 0 || index >= MAX_HANDLER_SIZE) {
            MELON_LOG(ERROR) << "CompressType=" << type << " is out of range";
            return nullptr;
        }
        if (nullptr == s_handler_map[index].Compress) {
            return nullptr;
        }
        return &s_handler_map[index];
    }

    const char *CompressTypeToCStr(CompressType type) {
        if (type == COMPRESS_TYPE_NONE) {
            return "none";
        }
        const CompressHandler *handler = FindCompressHandler(type);
        return (handler != nullptr ? handler->name : "unknown");
    }

    void ListCompressHandler(std::vector<CompressHandler> *vec) {
        vec->clear();
        for (int i = 0; i < MAX_HANDLER_SIZE; ++i) {
            if (s_handler_map[i].Compress != nullptr) {
                vec->push_back(s_handler_map[i]);
            }
        }
    }

    bool ParseFromCompressedData(const melon::cord_buf &data,
                                 google::protobuf::Message *msg,
                                 CompressType compress_type) {
        if (compress_type == COMPRESS_TYPE_NONE) {
            return ParsePbFromCordBuf(msg, data);
        }
        const CompressHandler *handler = FindCompressHandler(compress_type);
        if (nullptr != handler) {
            return handler->Decompress(data, msg);
        }
        return false;
    }

    bool SerializeAsCompressedData(const google::protobuf::Message &msg,
                                   melon::cord_buf *buf, CompressType compress_type) {
        if (compress_type == COMPRESS_TYPE_NONE) {
            melon::cord_buf_as_zero_copy_output_stream wrapper(buf);
            return msg.SerializeToZeroCopyStream(&wrapper);
        }
        const CompressHandler *handler = FindCompressHandler(compress_type);
        if (nullptr != handler) {
            return handler->Compress(msg, buf);
        }
        return false;
    }

} // namespace melon::rpc