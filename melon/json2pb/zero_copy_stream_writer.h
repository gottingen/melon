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

#include <google/protobuf/io/zero_copy_stream.h> // ZeroCopyOutputStream
#include <iostream>

//class IOBufAsZeroCopyOutputStream
//    : public google::protobuf::io::ZeroCopyOutputStream {
//public:
//    explicit IOBufAsZeroCopyOutputStream(IOBuf*);
//
//    // Interfaces of ZeroCopyOutputStream
//    bool Next(void** data, int* size);
//    void BackUp(int count);
//    google::protobuf::int64 ByteCount() const;
//
//private:
//    IOBuf* _buf;
//    size_t _initial_length;
//};

namespace json2pb {

    class ZeroCopyStreamWriter {
    public:
        typedef char Ch;

        ZeroCopyStreamWriter(google::protobuf::io::ZeroCopyOutputStream *stream)
                : _stream(stream), _data(nullptr),
                  _cursor(nullptr), _data_size(0) {
        }

        ~ZeroCopyStreamWriter() {
            if (_stream && _data) {
                _stream->BackUp(RemainSize());
            }
            _stream = nullptr;
        }

        void Put(char c) {
            if (__builtin_expect(AcquireNextBuf(), 1)) {
                *_cursor = c;
                ++_cursor;
            }
        }

        void PutN(char c, size_t n) {
            while (AcquireNextBuf() && n > 0) {
                size_t remain_size = RemainSize();
                size_t to_write = n > remain_size ? remain_size : n;
                memset(_cursor, c, to_write);
                _cursor += to_write;
                n -= to_write;
            }
        }

        void Puts(const char *str, size_t length) {
            while (AcquireNextBuf() && length > 0) {
                size_t remain_size = RemainSize();
                size_t to_write = length > remain_size ? remain_size : length;
                memcpy(_cursor, str, to_write);
                _cursor += to_write;
                str += to_write;
                length -= to_write;
            }
        }

        void Flush() {}

        // TODO: Add BIADU_CHECK
        char Peek() { return 0; }

        char Take() { return 0; }

        size_t Tell() { return 0; }

        char *PutBegin() { return nullptr; }

        size_t PutEnd(char *) { return 0; }

    private:
        bool AcquireNextBuf() {
            if (__builtin_expect(!_stream, 0)) {
                return false;
            }
            if (_data == nullptr || _cursor == _data + _data_size) {
                if (!_stream->Next((void **) &_data, &_data_size)) {
                    return false;
                }
                _cursor = _data;
            }
            return true;
        }

        size_t RemainSize() {
            return _data_size - (_cursor - _data);
        }

        google::protobuf::io::ZeroCopyOutputStream *_stream;
        char *_data;
        char *_cursor;
        int _data_size;
    };

}  // namespace json2pb
