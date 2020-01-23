//
// Created by liyinbin on 2019/12/8.
//
#include <abel/strings/case_conv.h>
#include <abel/strings/ascii.h>
namespace abel {

std::string &string_to_lower (std::string *str) {
    std::transform(str->begin(), str->end(), str->begin(),
                   [] (char c) { return ascii::to_lower(c); });
    return *str;
}

std::string &string_to_upper (std::string *str) {
    std::transform(str->begin(), str->end(), str->begin(),
                   [] (char c) { return ascii::to_upper(c); });
    return *str;
}

}