
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#pragma once

#include "melon/strings/utf/utf_selector.h"
#include "melon/strings/utf/utf_config.h"
#include <cstddef>
#include <iterator>
#include <string>
#include <string_view>

namespace melon {

    template<
            typename Utf,
            typename It>
    size_t char_size(It it) {
        return Utf::char_size([&it] { return *it; });
    }

    template<
            typename Utf,
            typename It>
    size_t size(It it) {
        size_t total_cp = 0;
        while (*it) {
            size_t size = Utf::char_size([&it] { return *it; });
            while (++it, --size > 0)
                if (!*it)
                    throw std::runtime_error("Not enough input for the null-terminated string");
            ++total_cp;
        }
        return total_cp;
    }

    namespace detail {

        enum struct iterator_impl {
            forward, random_access
        };

        template<
                typename It,
                iterator_impl>
        struct next_strategy final {
            void operator()(It &it, It const &eit, size_t size) {
                while (++it, --size > 0)
                    if (it == eit)
                        throw std::runtime_error("Not enough input for the forward iterator");
            }
        };

        template<typename It>
        struct next_strategy<It, iterator_impl::random_access> final {
            void operator()(It &it, It const &eit, typename std::iterator_traits<It>::difference_type const size) {
                if (eit - it < size)
                    throw std::runtime_error("Not enough input for the random access iterator");
                it += size;
            }
        };

    }

    template<
            typename Utf,
            typename It,
            typename Eit>
    size_t size(It it, Eit const eit) {
        size_t total_cp = 0;
        while (it != eit) {
            size_t const size = Utf::char_size([&it] { return *it; });
            detail::next_strategy<
                    typename std::decay<It>::type,
                    std::is_base_of<std::random_access_iterator_tag, typename std::iterator_traits<typename std::decay<It>::type>::iterator_category>::value
                    ? detail::iterator_impl::random_access
                    : detail::iterator_impl::forward>()(it, eit, size);
            ++total_cp;
        }
        return total_cp;
    }

    template<typename Ch>
    size_t size(Ch const *str) {
        return size<utf_selector_t<Ch>>
                (str);
    }

    template<typename Ch>
    size_t size(std::basic_string<Ch> str) {
        return size<utf_selector_t<Ch>>
                (str.cbegin(), str.cend());
    }


    template<typename Ch>
    size_t size(std::basic_string_view<Ch> str) {
        return size<utf_selector_t<Ch>>
                (str.cbegin(), str.cend());
    }


}  // namespace melon

