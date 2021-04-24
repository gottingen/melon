//
// Created by 李寅斌 on 2021/2/18.
//

#ifndef ABEL_UNICODE_TRAITS_H_
#define ABEL_UNICODE_TRAITS_H_

#include <cstddef>
#include <iterator>
#include <string>
#include <string_view>

#include "abel/unicode/utf8.h"
#include "abel/unicode/utf16.h"
#include "abel/unicode/utf32.h"
#include "abel/unicode/utfw.h"

namespace abel {

#ifndef ABEL_RAISE_UNICODE_ERRORS
    #define ABEL_RAISE_UNICODE_ERRORS 0
#endif

static uint32_t const max_unicode_code_point = 0x10FFFF;

namespace unicode_detail {

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

}  // namespace unicode_detail

template<typename Ch>
using utf_selector = unicode_detail::utf_selector<typename std::decay<Ch>::type>;

template<typename Ch>
using utf_selector_t = typename utf_selector<Ch>::type;


template<
        typename Utf,
        typename It>
size_t char_size(It it) {
    return Utf::char_size([&it] { return *it; });
}

template<
        typename Utf,
        typename It>
size_t unicode_size(It it) {
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

namespace unicode_detail {

enum struct iterator_impl {
    forward, random_access
};

template<typename It, iterator_impl>
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

}  // namespace detail

template<typename Utf, typename It, typename Eit>
size_t unicode_size(It it, Eit const eit) {
    size_t total_cp = 0;
    while (it != eit) {
        size_t const size = Utf::char_size([&it] { return *it; });
        unicode_detail::next_strategy<
                typename std::decay<It>::type,
                std::is_base_of<std::random_access_iterator_tag, typename std::iterator_traits<typename std::decay<It>::type>::iterator_category>::value
                ? unicode_detail::iterator_impl::random_access
                : unicode_detail::iterator_impl::forward>()(it, eit, size);
        ++total_cp;
    }
    return total_cp;
}

template<typename Ch>
size_t unicode_size(Ch const *str) {
    return unicode_size<utf_selector_t<Ch>>(str);
}

template<typename Ch>
size_t unicode_size(std::basic_string<Ch> str) {
    return unicode_size<utf_selector_t<Ch>>(str.cbegin(), str.cend());
}

template<typename Ch>
size_t unicode_size(std::basic_string_view<Ch> str) {
    return unicode_size<utf_selector_t<Ch>>(str.cbegin(), str.cend());
}

}


#endif  // ABEL_UNICODE_TRAITS_H_
