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


#ifndef MUTIL_RAW_PACK_H
#define MUTIL_RAW_PACK_H

#include <turbo/base/endian.h>

namespace mutil {

    // -------------------------------------------------------------------------
    // NOTE: RawPacker/RawUnpacker is used for packing/unpacking low-level and
    // hard-to-change header. If the fields are likely to be changed in future,
    // use protobuf.
    // -------------------------------------------------------------------------

    // This utility class packs 32-bit and 64-bit integers into binary data
    // that can be unpacked by RawUnpacker. Notice that the packed data is
    // schemaless and user must match pack..() methods with same-width
    // unpack..() methods to get the integers back.
    // Example:
    //   char buf[16];  // 4 + 8 + 4 bytes.
    //   mutil::RawPacker(buf).pack32(a).pack64(b).pack32(c);  // buf holds packed data
    //
    //   ... network ...
    //
    //   // positional correspondence with pack..()
    //   mutil::Unpacker(buf2).unpack32(a).unpack64(b).unpack32(c);
    class RawPacker {
    public:
        // Notice: User must guarantee `stream' is as long as the packed data.
        explicit RawPacker(void *stream) : _stream((char *) stream) {}

        ~RawPacker() {}

        // Not using operator<< because some values may be packed differently from
        // its type.
        RawPacker &pack32(uint32_t host_value) {
            *(uint32_t *) _stream = turbo::ghtonl(host_value);
            _stream += 4;
            return *this;
        }

        RawPacker &pack64(uint64_t host_value) {
            uint32_t *p = (uint32_t *) _stream;
            p[0] = turbo::ghtonl(host_value >> 32);
            p[1] = turbo::ghtonl(host_value & 0xFFFFFFFF);
            _stream += 8;
            return *this;
        }

    private:
        char *_stream;
    };

    // This utility class unpacks 32-bit and 64-bit integers from binary data
    // packed by RawPacker.
    class RawUnpacker {
    public:
        explicit RawUnpacker(const void *stream) : _stream((const char *) stream) {}

        ~RawUnpacker() {}

        RawUnpacker &unpack32(uint32_t &host_value) {
            host_value = turbo::gntohl(*(const uint32_t *) _stream);
            _stream += 4;
            return *this;
        }

        RawUnpacker &unpack64(uint64_t &host_value) {
            const uint32_t *p = (const uint32_t *) _stream;
            host_value = (((uint64_t) turbo::gntohl(p[0])) << 32) | turbo::gntohl(p[1]);
            _stream += 8;
            return *this;
        }

    private:
        const char *_stream;
    };

}  // namespace mutil

#endif  // MUTIL_RAW_PACK_H
