//
// Created by liyinbin on 2023/1/11.
//

#ifndef MELON_STRINGS_UTF8_NAIVE_DECODER_H_
#define MELON_STRINGS_UTF8_NAIVE_DECODER_H_

#include <cstddef>
#include <cstdint>

namespace melon::utf8_detail {

    ptrdiff_t naive_decoder(unsigned char const *s_ptr,
                           unsigned char const *s_ptr_end,
                           char32_t *dest) noexcept;

}  // namespace melon::utf8_detail
#endif  // MELON_STRINGS_UTF8_NAIVE_DECODER_H_
