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


// Date: Mon Feb  9 15:04:03 CST 2015

#include <stdio.h>
#include "melon/utility/status.h"

namespace mutil {

inline size_t status_size(size_t message_size) {
    // Add 1 because even if the sum of size is aligned with int, we need to
    // put an ending '\0'
    return ((offsetof(Status::State, message) + message_size)
            / sizeof(int) + 1) * sizeof(int);
}

int Status::set_errorv(int c, const char* fmt, va_list args) {
    if (0 == c) {
        free(_state);
        _state = NULL;
        return 0;
    }
    State* new_state = NULL;
    State* state = NULL;
    if (_state != NULL) {
        state = _state;
    } else {
        const size_t guess_size = std::max(strlen(fmt) * 2, 32UL);
        const size_t st_size = status_size(guess_size);
        new_state = reinterpret_cast<State*>(malloc(st_size));
        if (NULL == new_state) {
            return -1;
        }
        new_state->state_size = st_size;
        state = new_state;
    }
    const size_t cap = state->state_size - offsetof(State, message);
    va_list copied_args;
    va_copy(copied_args, args);
    const int bytes_used = vsnprintf(state->message, cap, fmt, copied_args);
    va_end(copied_args);
    if (bytes_used < 0) {
        free(new_state);
        return -1;
    } else if ((size_t)bytes_used < cap) {
        // There was enough room, just shrink and return.
        state->code = c;
        state->size = bytes_used;
        if (new_state == state) {
            _state = new_state;
        }
        return 0;
    } else {
        free(new_state);
        const size_t st_size = status_size(bytes_used);
        new_state = reinterpret_cast<State*>(malloc(st_size));
        if (NULL == new_state) {
            return -1;
        }
        new_state->code = c;
        new_state->size = bytes_used;
        new_state->state_size = st_size;
        const int bytes_used2 =
            vsnprintf(new_state->message, bytes_used + 1, fmt, args);
        if (bytes_used2 != bytes_used) {
            free(new_state);
            return -1;
        }
        free(_state);
        _state = new_state;
        return 0;
    }
}

int Status::set_error(int c, const mutil::StringPiece& error_msg) {
    if (0 == c) {
        free(_state);
        _state = NULL;
        return 0;
    }
    const size_t st_size = status_size(error_msg.size());
    if (_state == NULL || _state->state_size < st_size) {
        State* new_state = reinterpret_cast<State*>(malloc(st_size));
        if (NULL == new_state) {
            return -1;
        }
        new_state->state_size = st_size;
        free(_state);
        _state = new_state;
    }
    _state->code = c;
    _state->size = error_msg.size();
    memcpy(_state->message, error_msg.data(), error_msg.size());
    _state->message[error_msg.size()] = '\0';
    return 0;
}

Status::State* Status::copy_state(const State* s) {
    const size_t n = status_size(s->size);
    State* s2 = reinterpret_cast<State*>(malloc(n));
    if (NULL == s2) {
        // TODO: If we failed to allocate, the status will be OK.
        return NULL;
    }
    s2->code = s->code;
    s2->size = s->size;
    s2->state_size = n;
    char* msg_head = s2->message;
    memcpy(msg_head, s->message, s->size);
    msg_head[s->size] = '\0';
    return s2;
};

std::string Status::error_str() const {
    if (_state == NULL) {
        static std::string s_ok_str = "OK";
        return s_ok_str;
    }
    return std::string(_state->message, _state->size);
}

}  // namespace mutil
