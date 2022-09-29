
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/


#ifndef MELON_IO_RAW_PACK_H_
#define MELON_IO_RAW_PACK_H_

#include "melon/base/endian.h"

namespace melon {

    // -------------------------------------------------------------------------
    // NOTE: raw_packer/raw_unpacker is used for packing/unpacking low-level and
    // hard-to-change header. If the fields are likely to be changed in future,
    // use protobuf.
    // -------------------------------------------------------------------------

    // This utility class packs 32-bit and 64-bit integers into binary data
    // that can be unpacked by raw_unpacker. Notice that the packed data is
    // schemaless and user must match pack..() methods with same-width
    // unpack..() methods to get the integers back.
    // Example:
    //   char buf[16];  // 4 + 8 + 4 bytes.
    //   melon::raw_packer(buf).pack32(a).pack64(b).pack32(c);  // buf holds packed data
    //
    //   ... network ...
    //
    //   // positional correspondence with pack..()
    //   melon::raw_unpacker(buf2).unpack32(a).unpack64(b).unpack32(c);
    class raw_packer {
    public:
        // Notice: User must guarantee `stream' is as long as the packed data.
        explicit raw_packer(void *stream) : _stream((char *) stream) {}

        ~raw_packer() {}

        // Not using operator<< because some values may be packed differently from
        // its type.
        raw_packer &pack32(uint32_t host_value) {
            *(uint32_t *) _stream = melon::melon_hton32(host_value);
            _stream += 4;
            return *this;
        }

        raw_packer &pack64(uint64_t host_value) {
            uint32_t *p = (uint32_t *) _stream;
            p[0] = melon::melon_hton32(host_value >> 32);
            p[1] = melon::melon_hton32(host_value & 0xFFFFFFFF);
            _stream += 8;
            return *this;
        }

    private:
        char *_stream;
    };

    // This utility class unpacks 32-bit and 64-bit integers from binary data
    // packed by raw_packer.
    class raw_unpacker {
    public:
        explicit raw_unpacker(const void *stream) : _stream((const char *) stream) {}

        ~raw_unpacker() {}

        raw_unpacker &unpack32(uint32_t &host_value) {
            host_value = melon::melon_ntoh32(*(const uint32_t *) _stream);
            _stream += 4;
            return *this;
        }

        raw_unpacker &unpack64(uint64_t &host_value) {
            const uint32_t *p = (const uint32_t *) _stream;
            host_value = (((uint64_t) melon::melon_ntoh32(p[0])) << 32) | melon::melon_ntoh32(p[1]);
            _stream += 8;
            return *this;
        }

    private:
        const char *_stream;
    };

}  // namespace melon

#endif  // MELON_IO_RAW_PACK_H_
