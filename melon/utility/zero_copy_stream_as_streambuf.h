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


// Date: Thu Nov 22 13:57:56 CST 2012

#ifndef MUTIL_ZERO_COPY_STREAM_AS_STREAMBUF_H
#define MUTIL_ZERO_COPY_STREAM_AS_STREAMBUF_H

#include <streambuf>
#include <google/protobuf/io/zero_copy_stream.h>

namespace mutil {

// Wrap a ZeroCopyOutputStream into std::streambuf. Notice that before 
// destruction or shrink(), BackUp() of the stream are not called. In another
// word, if the stream is wrapped from IOBuf, the IOBuf may be larger than 
// appended data.
class ZeroCopyStreamAsStreamBuf : public std::streambuf {
public:
    ZeroCopyStreamAsStreamBuf(google::protobuf::io::ZeroCopyOutputStream* stream)
        : _zero_copy_stream(stream) {}
    virtual ~ZeroCopyStreamAsStreamBuf();

    // BackUp() unused bytes. Automatically called in destructor.
    void shrink();
    
protected:
    int overflow(int ch) override;
    int sync() override;
    std::streampos seekoff(std::streamoff off,
                           std::ios_base::seekdir way,
                           std::ios_base::openmode which) override;

private:
    google::protobuf::io::ZeroCopyOutputStream* _zero_copy_stream;
};

}  // namespace mutil

#endif  // MUTIL_ZERO_COPY_STREAM_AS_STREAMBUF_H
