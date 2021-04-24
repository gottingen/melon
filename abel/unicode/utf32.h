// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com

#ifndef ABEL_UNICODE_UTF32_H_
#define ABEL_UNICODE_UTF32_H_


#include <cstdint>
#include <stdexcept>

namespace abel {

struct utf32 final {
    static size_t const max_unicode_symbol_size = 1;
    static size_t const max_supported_symbol_size = 1;

    static uint32_t const max_supported_code_point = 0x7FFFFFFF;

    using char_type = uint32_t;

    template<typename PeekFn>
    static size_t char_size(PeekFn &&) {
        return 1;
    }

    template<typename ReadFn>
    static uint32_t read(ReadFn &&read_fn) {
        char_type const ch = std::forward<ReadFn>(read_fn)();
        if (ch < 0x80000000)
            return ch;
        throw std::runtime_error("Too large utf32 char");
    }

    template<typename WriteFn>
    static void write(uint32_t const cp, WriteFn &&write_fn) {
        if (cp < 0x80000000)
            std::forward<WriteFn>(write_fn)(static_cast<char_type>(cp));
        else
            throw std::runtime_error("Too large utf32 code point");
    }
};

}  // namespace abel

#endif  // ABEL_UNICODE_UTF32_H_
