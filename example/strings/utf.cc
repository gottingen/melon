//
// Created by liyinbin on 2022/9/29.
//
#include "melon/strings/utf.h"
#include <vector>
#include <string>

void test() {
    static char const u8s[] = "\xE0\xA4\xAF\xE0\xA5\x82\xE0\xA4\xA8\xE0\xA4\xBF\xE0\xA4\x95\xE0\xA5\x8B\xE0\xA4\xA1";
    using namespace melon;
    std::u16string u16;
    convz < utf_selector_t < decltype(*u8s) > , utf16 > (u8s, std::back_inserter(u16));
    std::u32string u32;
    conv < utf16, utf_selector_t < decltype(u32)::value_type >> (u16.begin(), u16.end(), std::back_inserter(u32));
    std::vector<char> u8;
    convz<utf32, utf8>(u32.data(), std::back_inserter(u8));
    std::wstring uw;
    conv<utf8, utfw>(u8s, u8s + sizeof(u8s), std::back_inserter(uw));
    auto u8r = conv<char>(uw);
    auto u16r = conv<char16_t>(u16);
    auto uwr = convz<wchar_t>(u8s);

    auto u32r = conv<char32_t>(std::string_view(u8r.data(), u8r.size())); // C++17 only

    static_assert(std::is_same < utf_selector < decltype(*u8s) > , utf_selector < decltype(u8)::value_type >> ::value,
                  "Fail");
    static_assert(
            std::is_same < utf_selector_t < decltype(u16)::value_type > ,
            utf_selector_t < decltype(uw)::value_type >> ::value !=
                    std::is_same < utf_selector_t < decltype(u32)::value_type > ,
            utf_selector_t < decltype(uw)::value_type >> ::value, "Fail");
}

int main() {
    test();
    return 0;
}