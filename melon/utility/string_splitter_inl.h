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


// Date: Mon. Apr. 18 19:52:34 CST 2011

#include <limits.h>

#ifndef MUTIL_STRING_SPLITTER_INL_H
#define MUTIL_STRING_SPLITTER_INL_H

namespace mutil {

StringSplitter::StringSplitter(const char* str_begin,
                               const char* str_end,
                               const char sep,
                               EmptyFieldAction action)
    : _head(str_begin)
    , _str_tail(str_end)
    , _sep(sep)
    , _empty_field_action(action) {
    init();
}

StringSplitter::StringSplitter(const char* str, char sep,
                               EmptyFieldAction action)
    : StringSplitter(str, NULL, sep, action) {}

StringSplitter::StringSplitter(const StringPiece& input, char sep,
                               EmptyFieldAction action)
    : StringSplitter(input.data(), input.data() + input.length(), sep, action) {}

void StringSplitter::init() {
    // Find the starting _head and _tail.
    if (__builtin_expect(_head != NULL, 1)) {
        if (_empty_field_action == SKIP_EMPTY_FIELD) {
            for (; _sep == *_head && not_end(_head); ++_head) {}
        }
        for (_tail = _head; *_tail != _sep && not_end(_tail); ++_tail) {}
    } else {
        _tail = NULL;
    }
}

StringSplitter& StringSplitter::operator++() {
    if (__builtin_expect(_tail != NULL, 1)) {
        if (not_end(_tail)) {
            ++_tail;
            if (_empty_field_action == SKIP_EMPTY_FIELD) {
                for (; _sep == *_tail && not_end(_tail); ++_tail) {}
            }
        }
        _head = _tail;
        for (; *_tail != _sep && not_end(_tail); ++_tail) {}
    }
    return *this;
}

StringSplitter StringSplitter::operator++(int) {
    StringSplitter tmp = *this;
    operator++();
    return tmp;
}

StringSplitter::operator const void*() const {
    return (_head != NULL && not_end(_head)) ? _head : NULL;
}

const char* StringSplitter::field() const {
    return _head;
}

size_t StringSplitter::length() const {
    return static_cast<size_t>(_tail - _head);
}

StringPiece StringSplitter::field_sp() const {
    return StringPiece(field(), length());
}

bool StringSplitter::not_end(const char* p) const {
    return (_str_tail == NULL) ? *p : (p != _str_tail);
}

int StringSplitter::to_int8(int8_t* pv) const {
    long v = 0;
    if (to_long(&v) == 0 && v >= -128 && v <= 127) {
        *pv = (int8_t)v;
        return 0;
    }
    return -1;
}

int StringSplitter::to_uint8(uint8_t* pv) const {
    unsigned long v = 0;
    if (to_ulong(&v) == 0 && v <= 255) {
        *pv = (uint8_t)v;
        return 0;
    }
    return -1;
}

int StringSplitter::to_int(int* pv) const {
    long v = 0;
    if (to_long(&v) == 0 && v >= INT_MIN && v <= INT_MAX) {
        *pv = (int)v;
        return 0;
    }
    return -1;
}

int StringSplitter::to_uint(unsigned int* pv) const {
    unsigned long v = 0;
    if (to_ulong(&v) == 0 && v <= UINT_MAX) {
        *pv = (unsigned int)v;
        return 0;
    }
    return -1;
}

int StringSplitter::to_long(long* pv) const {
    char* endptr = NULL;
    *pv = strtol(field(), &endptr, 10);
    return (endptr == field() + length()) ? 0 : -1;
}

int StringSplitter::to_ulong(unsigned long* pv) const {
    char* endptr = NULL;
    *pv = strtoul(field(), &endptr, 10);
    return (endptr == field() + length()) ? 0 : -1;
}

int StringSplitter::to_longlong(long long* pv) const {
    char* endptr = NULL;
    *pv = strtoll(field(), &endptr, 10);
    return (endptr == field() + length()) ? 0 : -1;
}

int StringSplitter::to_ulonglong(unsigned long long* pv) const {
    char* endptr = NULL;
    *pv = strtoull(field(), &endptr, 10);
    return (endptr == field() + length()) ? 0 : -1;
}

int StringSplitter::to_float(float* pv) const {
    char* endptr = NULL;
    *pv = strtof(field(), &endptr);
    return (endptr == field() + length()) ? 0 : -1;
}

int StringSplitter::to_double(double* pv) const {
    char* endptr = NULL;
    *pv = strtod(field(), &endptr);
    return (endptr == field() + length()) ? 0 : -1;
}

StringMultiSplitter::StringMultiSplitter (
    const char* str, const char* seps, EmptyFieldAction action)
    : _head(str)
    , _str_tail(NULL)
    , _seps(seps)
    , _empty_field_action(action) {
    init();
}

StringMultiSplitter::StringMultiSplitter (
    const char* str_begin, const char* str_end,
    const char* seps, EmptyFieldAction action)
    : _head(str_begin)
    , _str_tail(str_end)
    , _seps(seps)
    , _empty_field_action(action) {
    init();
}

void StringMultiSplitter::init() {
    if (__builtin_expect(_head != NULL, 1)) {
        if (_empty_field_action == SKIP_EMPTY_FIELD) {
            for (; is_sep(*_head) && not_end(_head); ++_head) {}
        }
        for (_tail = _head; !is_sep(*_tail) && not_end(_tail); ++_tail) {}
    } else {
        _tail = NULL;
    }
}

StringMultiSplitter& StringMultiSplitter::operator++() {
    if (__builtin_expect(_tail != NULL, 1)) {
        if (not_end(_tail)) {
            ++_tail;
            if (_empty_field_action == SKIP_EMPTY_FIELD) {
                for (; is_sep(*_tail) && not_end(_tail); ++_tail) {}
            }
        }
        _head = _tail;
        for (; !is_sep(*_tail) && not_end(_tail); ++_tail) {}
    }
    return *this;
}

StringMultiSplitter StringMultiSplitter::operator++(int) {
    StringMultiSplitter tmp = *this;
    operator++();
    return tmp;
}

bool StringMultiSplitter::is_sep(char c) const {
    for (const char* p = _seps; *p != '\0'; ++p) {
        if (c == *p) {
            return true;
        }
    }
    return false;
}

StringMultiSplitter::operator const void*() const {
    return (_head != NULL && not_end(_head)) ? _head : NULL;
}

const char* StringMultiSplitter::field() const {
    return _head;
}

size_t StringMultiSplitter::length() const {
    return static_cast<size_t>(_tail - _head);
}

StringPiece StringMultiSplitter::field_sp() const {
    return StringPiece(field(), length());
}

bool StringMultiSplitter::not_end(const char* p) const {
    return (_str_tail == NULL) ? *p : (p != _str_tail);
}

int StringMultiSplitter::to_int8(int8_t* pv) const {
    long v = 0;
    if (to_long(&v) == 0 && v >= -128 && v <= 127) {
        *pv = (int8_t)v;
        return 0;
    }
    return -1;
}

int StringMultiSplitter::to_uint8(uint8_t* pv) const {
    unsigned long v = 0;
    if (to_ulong(&v) == 0 && v <= 255) {
        *pv = (uint8_t)v;
        return 0;
    }
    return -1;
}

int StringMultiSplitter::to_int(int* pv) const {
    long v = 0;
    if (to_long(&v) == 0 && v >= INT_MIN && v <= INT_MAX) {
        *pv = (int)v;
        return 0;
    }
    return -1;
}

int StringMultiSplitter::to_uint(unsigned int* pv) const {
    unsigned long v = 0;
    if (to_ulong(&v) == 0 && v <= UINT_MAX) {
        *pv = (unsigned int)v;
        return 0;
    }
    return -1;
}

int StringMultiSplitter::to_long(long* pv) const {
    char* endptr = NULL;
    *pv = strtol(field(), &endptr, 10);
    return (endptr == field() + length()) ? 0 : -1;
}

int StringMultiSplitter::to_ulong(unsigned long* pv) const {
    char* endptr = NULL;
    *pv = strtoul(field(), &endptr, 10);
    return (endptr == field() + length()) ? 0 : -1;
}

int StringMultiSplitter::to_longlong(long long* pv) const {
    char* endptr = NULL;
    *pv = strtoll(field(), &endptr, 10);
    return (endptr == field() + length()) ? 0 : -1;
}

int StringMultiSplitter::to_ulonglong(unsigned long long* pv) const {
    char* endptr = NULL;
    *pv = strtoull(field(), &endptr, 10);
    return (endptr == field() + length()) ? 0 : -1;
}

int StringMultiSplitter::to_float(float* pv) const {
    char* endptr = NULL;
    *pv = strtof(field(), &endptr);
    return (endptr == field() + length()) ? 0 : -1;
}

int StringMultiSplitter::to_double(double* pv) const {
    char* endptr = NULL;
    *pv = strtod(field(), &endptr);
    return (endptr == field() + length()) ? 0 : -1;
}

void KeyValuePairsSplitter::UpdateDelimiterPosition() {
    const StringPiece key_value_pair(key_and_value());
    _delim_pos = key_value_pair.find(_key_value_delim);
    if (_delim_pos == StringPiece::npos) {
        _delim_pos = key_value_pair.length();
    }
}

}  // namespace mutil

#endif  // MUTIL_STRING_SPLITTER_INL_H
