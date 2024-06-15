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



#ifndef MELON_RPC_RTMP_AMF_INL_H_
#define MELON_RPC_RTMP_AMF_INL_H_

#include <turbo/base/endian.h>

void *fast_memcpy(void *__restrict dest, const void *__restrict src, size_t n);


namespace melon {

    inline size_t AMFInputStream::cutn(void *out, size_t n) {
        const size_t saved_n = n;
        do {
            if (_size >= (int64_t) n) {
                memcpy(out, _data, n);
                _data = (const char *) _data + n;
                _size -= n;
                _popped_bytes += saved_n;
                return saved_n;
            }
            if (_size) {
                memcpy(out, _data, _size);
                out = (char *) out + _size;
                n -= _size;
            }
        } while (_zc_stream->Next(&_data, &_size));
        _data = NULL;
        _size = 0;
        _popped_bytes += saved_n - n;
        return saved_n - n;
    }

    inline bool AMFInputStream::check_emptiness() {
        return _size == 0 && !_zc_stream->Next(&_data, &_size);
    }

    inline size_t AMFInputStream::cut_u8(uint8_t *val) {
        if (_size >= 1) {
            *val = *(uint8_t *) _data;
            _data = (const char *) _data + 1;
            _size -= 1;
            _popped_bytes += 1;
            return 1;
        }
        return cutn(val, 1);
    }

    inline size_t AMFInputStream::cut_u16(uint16_t *val) {
        if (_size >= 2) {
            const uint16_t netval = *(uint16_t *) _data;
            *val = turbo::gntohs(netval);
            _data = (const char *) _data + 2;
            _size -= 2;
            _popped_bytes += 2;
            return 2;
        }
        uint16_t netval = 0;
        const size_t ret = cutn(&netval, 2);
        *val = turbo::gntohs(netval);
        return ret;
    }

    inline size_t AMFInputStream::cut_u32(uint32_t *val) {
        if (_size >= 4) {
            const uint32_t netval = *(uint32_t *) _data;
            *val = turbo::gntohl(netval);
            _data = (const char *) _data + 4;
            _size -= 4;
            _popped_bytes += 4;
            return 4;
        }
        uint32_t netval = 0;
        const size_t ret = cutn(&netval, 4);
        *val =turbo::gntohl(netval);
        return ret;
    }

    inline size_t AMFInputStream::cut_u64(uint64_t *val) {
        if (_size >= 8) {
            const uint64_t netval = *(uint64_t *) _data;
            *val = turbo::gntohll(netval);
            _data = (const char *) _data + 8;
            _size -= 8;
            _popped_bytes += 8;
            return 8;
        }
        uint64_t netval = 0;
        const size_t ret = cutn(&netval, 8);
        *val = turbo::gntohll(netval);
        return ret;
    }

    inline void AMFOutputStream::done() {
        if (_good && _size) {
            _zc_stream->BackUp(_size);
            _size = 0;
        }
    }

    inline void AMFOutputStream::putn(const void *data, int n) {
        const int saved_n = n;
        do {
            if (n <= _size) {
                fast_memcpy(_data, data, n);
                _data = (char *) _data + n;
                _size -= n;
                _pushed_bytes += saved_n;
                return;
            }
            fast_memcpy(_data, data, _size);
            data = (const char *) data + _size;
            n -= _size;
        } while (_zc_stream->Next(&_data, &_size));
        _data = NULL;
        _size = 0;
        _pushed_bytes += (saved_n - n);
        if (n) {
            set_bad();
        }
    }

    inline void AMFOutputStream::put_u8(uint8_t val) {
        do {
            if (_size > 0) {
                *(uint8_t *) _data = val;
                _data = (char *) _data + 1;
                --_size;
                ++_pushed_bytes;
                return;
            }
        } while (_zc_stream->Next(&_data, &_size));
        _data = NULL;
        _size = 0;
        set_bad();
    }

    inline void AMFOutputStream::put_u16(uint16_t val) {
        uint16_t netval = turbo::ghtons(val);
        return putn(&netval, sizeof(netval));
    }

    inline void AMFOutputStream::put_u32(uint32_t val) {
        uint32_t netval = turbo::ghtonl(val);
        return putn(&netval, sizeof(netval));
    }

    inline void AMFOutputStream::put_u64(uint64_t val) {
        uint64_t netval = turbo::ghtonll(val);
        return putn(&netval, sizeof(netval));
    }

} // namespace melon


#endif  // MELON_RPC_RTMP_AMF_INL_H_
