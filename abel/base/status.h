// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com

#ifndef ABEL_BASE_STATUS_H_
#define ABEL_BASE_STATUS_H_

#include <string_view>
#include <algorithm>
#include <string>

namespace abel {

class status {
  public:
    // Create a success status.
    status() noexcept: _state(nullptr) {}

    ~status() { delete[] _state; }

    status(const status &rhs);

    status &operator=(const status &rhs);

    status(status &&rhs) noexcept: _state(rhs._state) { rhs._state = nullptr; }

    status &operator=(status &&rhs) noexcept;

    // Return a success status.
    static status ok() { return status(); }

    // Return error status of an appropriate type.
    static status not_found(const std::string_view &msg, const std::string_view &msg2 = std::string_view()) {
        return status(kNotFound, msg, msg2);
    }

    static status corruption(const std::string_view &msg, const std::string_view &msg2 = std::string_view()) {
        return status(kCorruption, msg, msg2);
    }

    static status not_supported(const std::string_view &msg, const std::string_view &msg2 = std::string_view()) {
        return status(kNotSupported, msg, msg2);
    }

    static status invalid_argument(const std::string_view &msg, const std::string_view &msg2 = std::string_view()) {
        return status(kInvalidArgument, msg, msg2);
    }

    static status io_error(const std::string_view &msg, const std::string_view &msg2 = std::string_view()) {
        return status(kIOError, msg, msg2);
    }

    // Returns true iff the status indicates success.
    bool is_ok() const { return (_state == nullptr); }

    // Returns true iff the status indicates a NotFound error.
    bool is_not_found() const { return code() == kNotFound; }

    // Returns true iff the status indicates a Corruption error.
    bool is_corruption() const { return code() == kCorruption; }

    // Returns true iff the status indicates an IOError.
    bool is_io_error() const { return code() == kIOError; }

    // Returns true iff the status indicates a NotSupportedError.
    bool is_not_supportedError() const { return code() == kNotSupported; }

    // Returns true iff the status indicates an InvalidArgument.
    bool is_invalid_argument() const { return code() == kInvalidArgument; }

    // Return a string representation of this status suitable for printing.
    // Returns the string "OK" for success.
    std::string to_string() const;

  private:
    enum result_code {
        kOk = 0,
        kNotFound = 1,
        kCorruption = 2,
        kNotSupported = 3,
        kInvalidArgument = 4,
        kIOError = 5
    };

    result_code code() const {
        return (_state == nullptr) ? kOk : static_cast<result_code>(_state[4]);
    }

    status(result_code code, const std::string_view &msg, const std::string_view &msg2);

    static const char *copy_state(const char *s);

    // OK status has a null _state.  Otherwise, _state is a new[] array
    // of the following form:
    //    _state[0..3] == length of message
    //    _state[4]    == code
    //    _state[5..]  == message
    const char *_state;
};

inline status::status(const status &rhs) {
    _state = (rhs._state == nullptr) ? nullptr : copy_state(rhs._state);
}

inline status &status::operator=(const status &rhs) {
    // The following condition catches both aliasing (when this == &rhs),
    // and the common case where both rhs and *this are ok.
    if (_state != rhs._state) {
        delete[] _state;
        _state = (rhs._state == nullptr) ? nullptr : copy_state(rhs._state);
    }
    return *this;
}

inline status &status::operator=(status &&rhs) noexcept {
    std::swap(_state, rhs._state);
    return *this;
}

}  // namespace abel

#endif //ABEL_BASE_STATUS_H_
