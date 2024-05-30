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



#ifndef MELON_RPC_RTMP_RTMP_UTILS_H_
#define MELON_RPC_RTMP_RTMP_UTILS_H_

#include <stdint.h>  // int32_t
#include <stddef.h>  // size_t

namespace melon {

// Extract bits one by one from the bytes.
    class BitStream {
    public:
        BitStream(const void *data, size_t len)
                : _data(data), _data_end((const char *) data + len), _shift(7) {}

        ~BitStream() {}

        // True if no bits any more.
        bool empty() { return _data == _data_end; }

        // Read one bit from the data. empty() must be checked before calling
        // this function, otherwise the behavior is undefined.
        int8_t read_bit() {
            const int8_t *p = (const int8_t *) _data;
            int8_t result = (*p >> _shift) & 0x1;
            if (_shift == 0) {
                _shift = 7;
                _data = p + 1;
            } else {
                --_shift;
            }
            return result;
        }

    private:
        const void *_data;
        const void *const _data_end;
        int _shift;
    };

    int avc_nalu_read_uev(BitStream *stream, int32_t *v);

    int avc_nalu_read_bit(BitStream *stream, int8_t *v);

} // namespace melon


#endif  // MELON_RPC_RTMP_RTMP_UTILS_H_
