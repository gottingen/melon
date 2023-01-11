//
// Created by liyinbin on 2023/1/11.
//

#ifndef MELON_STRINGS_UTF8_GREEDY_ENCODER_H_
#define MELON_STRINGS_UTF8_GREEDY_ENCODER_H_

#include <cstddef>
#include <cstdint>

namespace melon::utf8_detail {

    size_t greedy_count_bytes_size(const uint32_t *s_ptr, const uint32_t *s_ptr_end) noexcept;

    ptrdiff_t greedy_encoder(const uint32_t *s_ptr, const uint32_t *s_ptr_end, unsigned char *dst) noexcept;

}  // namespace melon::utf8_detail
#endif  // MELON_STRINGS_UTF8_GREEDY_ENCODER_H_
