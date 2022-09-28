
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#pragma once

#include "melon/strings/utf/cp_utf8.h"
#include "melon/strings/utf/cp_utf16.h"
#include "melon/strings/utf/cp_utf32.h"
#include "melon/strings/utf/cp_utfw.h"

namespace melon {
    namespace detail {

        template<typename Ch>
        struct utf_selector final {
        };

        template<>
        struct utf_selector<char> final {
            using type = utf8;
        };
        template<>
        struct utf_selector<unsigned char> final {
            using type = utf8;
        };
        template<>
        struct utf_selector<signed char> final {
            using type = utf8;
        };
        template<>
        struct utf_selector<char16_t> final {
            using type = utf16;
        };
        template<>
        struct utf_selector<char32_t> final {
            using type = utf32;
        };
        template<>
        struct utf_selector<wchar_t> final {
            using type = utfw;
        };

    }

    template<typename Ch>
    using utf_selector = detail::utf_selector<typename std::decay<Ch>::type>;

    template<typename Ch>
    using utf_selector_t = typename utf_selector<Ch>::type;

}  // namespace melon

