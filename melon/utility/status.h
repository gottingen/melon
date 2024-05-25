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


#ifndef MUTIL_STATUS_H
#define MUTIL_STATUS_H

#include <stdarg.h>                       // va_list
#include <stdlib.h>                       // free
#include <string>                         // std::string
#include <ostream>                        // std::ostream
#include <melon/utility/strings/string_piece.h>

namespace mutil {

// A Status encapsulates the result of an operation. It may indicate success,
// or it may indicate an error with an associated error message. It's suitable
// for passing status of functions with richer information than just error_code
// in exception-forbidden code. This utility is inspired by leveldb::Status.
//
// Multiple threads can invoke const methods on a Status without
// external synchronization, but if any of the threads may call a
// non-const method, all threads accessing the same Status must use
// external synchronization.
//
// Since failed status needs to allocate memory, you should be careful when
// failed status is frequent.

class Status {
public:
    struct State {
        int code;
        unsigned size;  // length of message string
        unsigned state_size;
        char message[0];
    };

    // Create a success status.
    Status() : _state(NULL) { }
    // Return a success status.
    static Status OK() { return Status(); }

    ~Status() { reset(); }

    // Create a failed status.
    // error_text is formatted from `fmt' and following arguments.
    Status(int code, const char* fmt, ...) 
        __attribute__ ((__format__ (__printf__, 3, 4)))
        : _state(NULL) {
        va_list ap;
        va_start(ap, fmt);
        set_errorv(code, fmt, ap);
        va_end(ap);
    }
    Status(int code, const mutil::StringPiece& error_msg) : _state(NULL) {
        set_error(code, error_msg);
    }

    // Copy the specified status. Internal fields are deeply copied.
    Status(const Status& s);
    void operator=(const Status& s);

    // Reset this status to be OK.
    void reset();
    
    // Reset this status to be failed.
    // Returns 0 on success, -1 otherwise and internal fields are not changed.
    int set_error(int code, const char* error_format, ...)
        __attribute__ ((__format__ (__printf__, 3, 4)));
    int set_error(int code, const mutil::StringPiece& error_msg);
    int set_errorv(int code, const char* error_format, va_list args);

    // Returns true iff the status indicates success.
    bool ok() const { return (_state == NULL); }

    // Get the error code
    int error_code() const {
        return (_state == NULL) ? 0 : _state->code;
    }

    // Return a string representation of the status.
    // Returns "OK" for success.
    // NOTICE:
    //   * You can print a Status to std::ostream directly
    //   * if message contains '\0', error_cstr() will not be shown fully.
    const char* error_cstr() const {
        return (_state == NULL ? "OK" : _state->message);
    }
    mutil::StringPiece error_data() const {
        return (_state == NULL ? mutil::StringPiece("OK", 2)
                : mutil::StringPiece(_state->message, _state->size));
    }
    std::string error_str() const;

    void swap(mutil::Status& other) { std::swap(_state, other._state); }

private:    
    // OK status has a NULL _state.  Otherwise, _state is a State object
    // converted from malloc().
    State* _state;

    static State* copy_state(const State* s);
};

inline Status::Status(const Status& s) {
    _state = (s._state == NULL) ? NULL : copy_state(s._state);
}

inline int Status::set_error(int code, const char* msg, ...) {
    va_list ap;
    va_start(ap, msg);
    const int rc = set_errorv(code, msg, ap);
    va_end(ap);
    return rc;
}

inline void Status::reset() {
    free(_state);
    _state = NULL;
}

inline void Status::operator=(const Status& s) {
    // The following condition catches both aliasing (when this == &s),
    // and the common case where both s and *this are ok.
    if (_state == s._state) {
        return;
    }
    if (s._state == NULL) {
        free(_state);
        _state = NULL;
    } else {
        set_error(s._state->code,
                  mutil::StringPiece(s._state->message, s._state->size));
    }
}

inline std::ostream& operator<<(std::ostream& os, const Status& st) {
    // NOTE: don't use st.error_text() which is inaccurate if message has '\0'
    return os << st.error_data();
}

}  // namespace mutil

#endif  // MUTIL_STATUS_H
