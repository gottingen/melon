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


#ifndef  MELON_JSON2PB_ZERO_COPY_STREAM_READER_H_
#define  MELON_JSON2PB_ZERO_COPY_STREAM_READER_H_

#include <google/protobuf/io/zero_copy_stream.h> // ZeroCopyInputStream

namespace json2pb {

    class ZeroCopyStreamReader {
    public:
        typedef char Ch;

        ZeroCopyStreamReader(google::protobuf::io::ZeroCopyInputStream *stream)
                : _data(nullptr), _data_size(0), _nread(0), _stream(stream) {
        }

        //Take a charactor and return its address.
        const char *PeekAddr() {
            if (!ReadBlockTail()) {
                return _data;
            }
            while (_stream->Next((const void **) &_data, &_data_size)) {
                if (!ReadBlockTail()) {
                    return _data;
                }
            }
            return nullptr;
        }

        const char *TakeWithAddr() {
            const char *c = PeekAddr();
            if (c) {
                ++_nread;
                --_data_size;
                return _data++;
            }
            return nullptr;
        }

        char Take() {
            const char *c = PeekAddr();
            if (c) {
                ++_nread;
                --_data_size;
                ++_data;
                return *c;
            }
            return '\0';
        }

        char Peek() {
            const char *c = PeekAddr();
            return (c ? *c : '\0');
        }

        //Tell whether read the end of this block.
        bool ReadBlockTail() {
            return !_data_size;
        }

        size_t Tell() { return _nread; }

        void Put(char) {}

        void Flush() {}

        char *PutBegin() { return nullptr; }

        size_t PutEnd(char *) { return 0; }

    private:
        const char *_data;
        int _data_size;
        size_t _nread;
        google::protobuf::io::ZeroCopyInputStream *_stream;
    };

} // namespace json2pb

#endif  // MELON_JSON2PB_ZERO_COPY_STREAM_READER_H_
