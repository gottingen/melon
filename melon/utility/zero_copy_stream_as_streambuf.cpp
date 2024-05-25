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

#include <melon/utility/macros.h>
#include "melon/utility/zero_copy_stream_as_streambuf.h"

namespace mutil {

MELON_CASSERT(sizeof(std::streambuf::char_type) == sizeof(char),
              only_support_char);

int ZeroCopyStreamAsStreamBuf::overflow(int ch) {
    if (ch == std::streambuf::traits_type::eof()) {
        return ch;
    }
    void* block = NULL;
    int size = 0;
    if (_zero_copy_stream->Next(&block, &size)) {
        setp((char*)block, (char*)block + size);
        // if size == 0, this function will call overflow again.
        return sputc(ch);
    } else {
        setp(NULL, NULL);
        return std::streambuf::traits_type::eof();
    }
}

int ZeroCopyStreamAsStreamBuf::sync() {
    // data are already in IOBuf.
    return 0;
}

ZeroCopyStreamAsStreamBuf::~ZeroCopyStreamAsStreamBuf() {
    shrink();
}

void ZeroCopyStreamAsStreamBuf::shrink() {
    if (pbase() != NULL) {
        _zero_copy_stream->BackUp(epptr() - pptr());
        setp(NULL, NULL);
    }
}

std::streampos ZeroCopyStreamAsStreamBuf::seekoff(
    std::streamoff off,
    std::ios_base::seekdir way,
    std::ios_base::openmode which) {
    if (off == 0 && way == std::ios_base::cur) {
        return _zero_copy_stream->ByteCount() - (epptr() - pptr());
    }
    return (std::streampos)(std::streamoff)-1;
}


}  // namespace mutil
