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
