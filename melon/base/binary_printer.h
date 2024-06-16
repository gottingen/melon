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

#ifndef MUTIL_BINARY_PRINTER_H
#define MUTIL_BINARY_PRINTER_H

#include <string_view>

namespace mutil {
    class IOBuf;

    // Print binary content within max length.
    // The printing format is optimized for humans and may change in future.

    class ToPrintable {
    public:
        static const size_t DEFAULT_MAX_LENGTH = 64;

        ToPrintable(const IOBuf &b, size_t max_length = DEFAULT_MAX_LENGTH)
                : _iobuf(&b), _max_length(max_length) {}

        ToPrintable(const std::string_view &str, size_t max_length = DEFAULT_MAX_LENGTH)
                : _iobuf(NULL), _str(str), _max_length(max_length) {}

        ToPrintable(const void *data, size_t n, size_t max_length = DEFAULT_MAX_LENGTH)
                : _iobuf(NULL), _str((const char *) data, n), _max_length(max_length) {}

        void Print(std::ostream &os) const;

    private:
        const IOBuf *_iobuf;
        std::string_view _str;
        size_t _max_length;
    };

    // Keep old name for compatibility.
    typedef ToPrintable PrintedAsBinary;

    inline std::ostream &operator<<(std::ostream &os, const ToPrintable &p) {
        p.Print(os);
        return os;
    }

    // Convert binary data to a printable string.
    std::string ToPrintableString(const IOBuf &data,
                                  size_t max_length = ToPrintable::DEFAULT_MAX_LENGTH);

    std::string ToPrintableString(const std::string_view &data,
                                  size_t max_length = ToPrintable::DEFAULT_MAX_LENGTH);

    std::string ToPrintableString(const void *data, size_t n,
                                  size_t max_length = ToPrintable::DEFAULT_MAX_LENGTH);

} // namespace mutil

#endif  // MUTIL_BINARY_PRINTER_H
