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



#include <limits>
#include <turbo/log/logging.h>
#include <melon/utility/string_printf.h>
#include <melon/rpc/redis/redis_reply.h>

namespace melon {

//MELON_CASSERT(sizeof(RedisReply) == 24, size_match);
const int RedisReply::npos = -1;

const char* RedisReplyTypeToString(RedisReplyType type) {
    switch (type) {
    case REDIS_REPLY_STRING: return "string";
    case REDIS_REPLY_ARRAY: return "array";
    case REDIS_REPLY_INTEGER: return "integer";
    case REDIS_REPLY_NIL: return "nil";
    case REDIS_REPLY_STATUS: return "status";
    case REDIS_REPLY_ERROR: return "error";
    default: return "unknown redis type";
    }
}

bool RedisReply::SerializeTo(mutil::IOBufAppender* appender) {
    switch (_type) {
        case REDIS_REPLY_ERROR:
            // fall through
        case REDIS_REPLY_STATUS:
            appender->push_back((_type == REDIS_REPLY_ERROR)? '-' : '+');
            if (_length < (int)sizeof(_data.short_str)) {
                appender->append(_data.short_str, _length);
            } else {
                appender->append(_data.long_str, _length);
            }
            appender->append("\r\n", 2);
            return true;
        case REDIS_REPLY_INTEGER:
            appender->push_back(':');
            appender->append_decimal(_data.integer);
            appender->append("\r\n", 2);
            return true;
        case REDIS_REPLY_STRING:
            appender->push_back('$');
            appender->append_decimal(_length);
            appender->append("\r\n", 2);
            if (_length != npos) {
                if (_length < (int)sizeof(_data.short_str)) {
                    appender->append(_data.short_str, _length);
                } else {
                    appender->append(_data.long_str, _length);
                }
                appender->append("\r\n", 2);
            }
            return true;
        case REDIS_REPLY_ARRAY:
            appender->push_back('*');
            appender->append_decimal(_length);
            appender->append("\r\n", 2);
            if (_length != npos) {
                for (int i = 0; i < _length; ++i) {
                    if (!_data.array.replies[i].SerializeTo(appender)) {
                        return false;
                    }
                }
            }
            return true;
        case REDIS_REPLY_NIL:
            LOG(ERROR) << "Do you forget to call SetXXX()?";
            return false;
    }
    CHECK(false) << "unknown redis type=" << _type;
    return false;
}

ParseError RedisReply::ConsumePartialIOBuf(mutil::IOBuf& buf) {
    if (_type == REDIS_REPLY_ARRAY && _data.array.last_index >= 0) {
        // The parsing was suspended while parsing sub replies,
        // continue the parsing.
        RedisReply* subs = (RedisReply*)_data.array.replies;
        for (int i = _data.array.last_index; i < _length; ++i) {
            ParseError err = subs[i].ConsumePartialIOBuf(buf);
            if (err != PARSE_OK) {
                return err;
            }
            ++_data.array.last_index;
        }
        // We've got an intact reply. reset the index.
        _data.array.last_index = -1;
        return PARSE_OK;
    }

    // Notice that all branches returning PARSE_ERROR_NOT_ENOUGH_DATA must not change `buf'.
    const char* pfc = (const char*)buf.fetch1();
    if (pfc == NULL) {
        return PARSE_ERROR_NOT_ENOUGH_DATA;
    }
    const char fc = *pfc;  // first character
    switch (fc) {
    case '-':   // Error          "-<message>\r\n"
    case '+': { // Simple String  "+<string>\r\n"
        mutil::IOBuf str;
        if (buf.cut_until(&str, "\r\n") != 0) {
            const size_t len = buf.size();
            if (len > std::numeric_limits<uint32_t>::max()) {
                LOG(ERROR) << "simple string is too long! max length=2^32-1,"
                              " actually=" << len;
                return PARSE_ERROR_ABSOLUTELY_WRONG;
            }
            return PARSE_ERROR_NOT_ENOUGH_DATA;
        }
        const size_t len = str.size() - 1;
        if (len < sizeof(_data.short_str)) {
            // SSO short strings, including empty string.
            _type = (fc == '-' ? REDIS_REPLY_ERROR : REDIS_REPLY_STATUS);
            _length = len;
            str.copy_to_cstr(_data.short_str, (size_t)-1L, 1/*skip fc*/);
            return PARSE_OK;
        }
        char* d = (char*)_arena->allocate((len/8 + 1)*8);
        if (d == NULL) {
            LOG(FATAL) << "Fail to allocate string[" << len << "]";
            return PARSE_ERROR_ABSOLUTELY_WRONG;
        }
        CHECK_EQ(len, str.copy_to_cstr(d, (size_t)-1L, 1/*skip fc*/));
        _type = (fc == '-' ? REDIS_REPLY_ERROR : REDIS_REPLY_STATUS);
        _length = len;
        _data.long_str = d;
        return PARSE_OK;
    }
    case '$':   // Bulk String   "$<length>\r\n<string>\r\n"
    case '*':   // Array         "*<size>\r\n<sub-reply1><sub-reply2>..."
    case ':': { // Integer       ":<integer>\r\n"
        char intbuf[32];  // enough for fc + 64-bit decimal + \r\n
        const size_t ncopied = buf.copy_to(intbuf, sizeof(intbuf) - 1);
        intbuf[ncopied] = '\0';
        const size_t crlf_pos = mutil::StringPiece(intbuf, ncopied).find("\r\n");
        if (crlf_pos == mutil::StringPiece::npos) {  // not enough data
            return PARSE_ERROR_NOT_ENOUGH_DATA;
        }
        char* endptr = NULL;
        int64_t value = strtoll(intbuf + 1/*skip fc*/, &endptr, 10);
        if (endptr != intbuf + crlf_pos) {
            LOG(ERROR) << '`' << intbuf + 1 << "' is not a valid 64-bit decimal";
            return PARSE_ERROR_ABSOLUTELY_WRONG;
        }
        if (fc == ':') {
            buf.pop_front(crlf_pos + 2/*CRLF*/);
            _type = REDIS_REPLY_INTEGER;
            _length = 0;
            _data.integer = value;
            return PARSE_OK;
        } else if (fc == '$') {
            const int64_t len = value;  // `value' is length of the string
            if (len < 0) {  // redis nil
                buf.pop_front(crlf_pos + 2/*CRLF*/);
                _type = REDIS_REPLY_NIL;
                _length = 0;
                _data.integer = 0;
                return PARSE_OK;
            }
            if (len > (int64_t)std::numeric_limits<uint32_t>::max()) {
                LOG(ERROR) << "bulk string is too long! max length=2^32-1,"
                    " actually=" << len;
                return PARSE_ERROR_ABSOLUTELY_WRONG;
            }
            // We provide c_str(), thus even if bulk string is started with
            // length, we have to end it with \0.
            if (buf.size() < crlf_pos + 2 + (size_t)len + 2/*CRLF*/) {
                return PARSE_ERROR_NOT_ENOUGH_DATA;
            }
            if ((size_t)len < sizeof(_data.short_str)) {
                // SSO short strings, including empty string.
                _type = REDIS_REPLY_STRING;
                _length = len;
                buf.pop_front(crlf_pos + 2);
                buf.cutn(_data.short_str, len);
                _data.short_str[len] = '\0';
            } else {
                char* d = (char*)_arena->allocate((len/8 + 1)*8);
                if (d == NULL) {
                    LOG(FATAL) << "Fail to allocate string[" << len << "]";
                    return PARSE_ERROR_ABSOLUTELY_WRONG;
                }
                buf.pop_front(crlf_pos + 2/*CRLF*/);
                buf.cutn(d, len);
                d[len] = '\0';
                _type = REDIS_REPLY_STRING;
                _length = len;
                _data.long_str = d;
            }
            char crlf[2];
            buf.cutn(crlf, sizeof(crlf));
            if (crlf[0] != '\r' || crlf[1] != '\n') {
                LOG(ERROR) << "Bulk string is not ended with CRLF";
                return PARSE_ERROR_ABSOLUTELY_WRONG;
            }
            return PARSE_OK;
        } else {
            const int64_t count = value;  // `value' is count of sub replies
            if (count < 0) { // redis nil
                buf.pop_front(crlf_pos + 2/*CRLF*/);
                _type = REDIS_REPLY_NIL;
                _length = 0;
                _data.integer = 0;
                return PARSE_OK;
            }
            if (count == 0) { // empty array
                buf.pop_front(crlf_pos + 2/*CRLF*/);
                _type = REDIS_REPLY_ARRAY;
                _length = 0;
                _data.array.last_index = -1;
                _data.array.replies = NULL;
                return PARSE_OK;
            }
            if (count > (int64_t)std::numeric_limits<uint32_t>::max()) {
                LOG(ERROR) << "Too many sub replies! max count=2^32-1,"
                    " actually=" << count;
                return PARSE_ERROR_ABSOLUTELY_WRONG;
            }
            // FIXME(gejun): Call allocate_aligned instead.
            RedisReply* subs = (RedisReply*)_arena->allocate(sizeof(RedisReply) * count);
            if (subs == NULL) {
                LOG(FATAL) << "Fail to allocate RedisReply[" << count << "]";
                return PARSE_ERROR_ABSOLUTELY_WRONG;
            }
            for (int64_t i = 0; i < count; ++i) {
                new (&subs[i]) RedisReply(_arena);
            }
            buf.pop_front(crlf_pos + 2/*CRLF*/);
            _type = REDIS_REPLY_ARRAY;
            _length = count;
            _data.array.replies = subs;

            // Recursively parse sub replies. If any of them fails, it will
            // be continued in next calls by tracking _data.array.last_index.
            _data.array.last_index = 0;
            for (int64_t i = 0; i < count; ++i) {
                ParseError err = subs[i].ConsumePartialIOBuf(buf);
                if (err != PARSE_OK) {
                    return err;
                }
                ++_data.array.last_index;
            }
            _data.array.last_index = -1;
            return PARSE_OK;
        }
    }
    default:
        LOG(ERROR) << "Invalid first character=" << (int)fc;
        return PARSE_ERROR_ABSOLUTELY_WRONG;
    }
    return PARSE_ERROR_ABSOLUTELY_WRONG;
}

class RedisStringPrinter {
public:
    RedisStringPrinter(const char* str, size_t length)
        : _str(str, length) {}
    void Print(std::ostream& os) const;
private:
    mutil::StringPiece _str;
};

static std::ostream&
operator<<(std::ostream& os, const RedisStringPrinter& printer) {
    printer.Print(os);
    return os;
}

void RedisStringPrinter::Print(std::ostream& os) const {
    size_t flush_start = 0;
    for (size_t i = 0; i < _str.size(); ++i) {
        const char c = _str[i];
        if (c <= 0) { // unprintable chars
            if (i != flush_start) {
                os << mutil::StringPiece(_str.data() + flush_start, i - flush_start);
            }
            char buf[8] = "\\u0000";
            uint8_t d1 = ((uint8_t)c) & 0xF;
            uint8_t d2 = ((uint8_t)c) >> 4;
            buf[4] = (d1 < 10 ? d1 + '0' : (d1 - 10) + 'A');
            buf[5] = (d2 < 10 ? d2 + '0' : (d2 - 10) + 'A');
            os << mutil::StringPiece(buf, 6);
            flush_start = i + 1;
        } else if (c == '"' || c == '\\') {  // need to escape
            if (i != flush_start) {
                os << mutil::StringPiece(_str.data() + flush_start, i - flush_start);
            }
            os << '\\' << c;
            flush_start = i + 1;
        }
    }
    if (flush_start != _str.size()) {
        os << mutil::StringPiece(_str.data() + flush_start, _str.size() - flush_start);
    }
}

// Mimic how official redis-cli prints.
void RedisReply::Print(std::ostream& os) const {
    switch (_type) {
    case REDIS_REPLY_STRING:
        os << '"';
        if (_length < (int)sizeof(_data.short_str)) {
            os << RedisStringPrinter(_data.short_str, _length);
        } else {
            os << RedisStringPrinter(_data.long_str, _length);
        }
        os << '"';
        break;
    case REDIS_REPLY_ARRAY:
        os << '[';
        for (int i = 0; i < _length; ++i) {
            if (i != 0) {
                os << ", ";
            }
            _data.array.replies[i].Print(os);
        }
        os << ']';
        break;
    case REDIS_REPLY_INTEGER:
        os << "(integer) " << _data.integer;
        break;
    case REDIS_REPLY_NIL:
        os << "(nil)";
        break;
    case REDIS_REPLY_ERROR:
        os << "(error) ";
        // fall through
    case REDIS_REPLY_STATUS:
        if (_length < (int)sizeof(_data.short_str)) {
            os << RedisStringPrinter(_data.short_str, _length);
        } else {
            os << RedisStringPrinter(_data.long_str, _length);
        }
        break;
    default:
        os << "UnknownType=" << _type;
        break;
    }
}

void RedisReply::CopyFromDifferentArena(const RedisReply& other) {
    _type = other._type;
    _length = other._length;
    switch (_type) {
    case REDIS_REPLY_ARRAY: {
        RedisReply* subs = (RedisReply*)_arena->allocate(sizeof(RedisReply) * _length);
        if (subs == NULL) {
            LOG(FATAL) << "Fail to allocate RedisReply[" << _length << "]";
            return;
        }
        for (int i = 0; i < _length; ++i) {
            new (&subs[i]) RedisReply(_arena);
        }
        _data.array.last_index = other._data.array.last_index;
        if (_data.array.last_index > 0) {
            // incomplete state
            for (int i = 0; i < _data.array.last_index; ++i) {
                subs[i].CopyFromDifferentArena(other._data.array.replies[i]);
            }
        } else {
            for (int i = 0; i < _length; ++i) {
                subs[i].CopyFromDifferentArena(other._data.array.replies[i]);
            }
        }
        _data.array.replies = subs;
    }
        break;
    case REDIS_REPLY_INTEGER:
        _data.integer = other._data.integer;
        break;
    case REDIS_REPLY_NIL:
        break;
    case REDIS_REPLY_STRING:
        // fall through
    case REDIS_REPLY_ERROR:
        // fall through
    case REDIS_REPLY_STATUS:
        if (_length < (int)sizeof(_data.short_str)) {
            memcpy(_data.short_str, other._data.short_str, _length + 1);
        } else {
            char* d = (char*)_arena->allocate((_length/8 + 1)*8);
            if (d == NULL) {
                LOG(FATAL) << "Fail to allocate string[" << _length << "]";
                return;
            }
            memcpy(d, other._data.long_str, _length + 1);
            _data.long_str = d;
        }
        break;
    }
}

void RedisReply::SetArray(int size) {
    if (_type != REDIS_REPLY_NIL) {
        Reset();
    }
    _type = REDIS_REPLY_ARRAY;
    if (size < 0) {
        LOG(ERROR) << "negative size=" << size << " when calling SetArray";
        return;
    } else if (size == 0) {
        _length = 0;
        return;
    }
    RedisReply* subs = (RedisReply*)_arena->allocate(sizeof(RedisReply) * size);
    if (!subs) {
        LOG(FATAL) << "Fail to allocate RedisReply[" << size << "]";
        return;
    }
    for (int i = 0; i < size; ++i) {
        new (&subs[i]) RedisReply(_arena);
    }
    _length = size;
    _data.array.replies = subs;
}

void RedisReply::SetStringImpl(const mutil::StringPiece& str, RedisReplyType type) {
    if (_type != REDIS_REPLY_NIL) {
        Reset();
    }
    const size_t size = str.size();
    if (size < sizeof(_data.short_str)) {
        memcpy(_data.short_str, str.data(), size);
        _data.short_str[size] = '\0';
    } else {
        char* d = (char*)_arena->allocate((size/8 + 1) * 8);
        if (!d) {
            LOG(FATAL) << "Fail to allocate string[" << size << "]";
            return;
        }
        memcpy(d, str.data(), size);
        d[size] = '\0';
        _data.long_str = d;
    }
    _type = type;
    _length = size;
}

void RedisReply::FormatStringImpl(const char* fmt, va_list args, RedisReplyType type) {
    va_list copied_args;
    va_copy(copied_args, args);
    char buf[64];
    int ret = vsnprintf(buf, sizeof(buf), fmt, copied_args);
    va_end(copied_args);
    if (ret < 0) {
        LOG(FATAL) << "Fail to vsnprintf into buf=" << (void*)buf << " size=" << sizeof(buf);
        return;
    } else if (ret < (int)sizeof(buf)) {
        return SetStringImpl(buf, type);
    } else {
        std::string str;
        str.reserve(ret + 1);
        mutil::string_vappendf(&str, fmt, args);
        return SetStringImpl(str, type);
    }
}

} // namespace melon
