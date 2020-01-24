//
// Created by liyinbin on 2019/12/8.
//

#ifndef ABEL_STRING_END_WITH_H_
#define ABEL_STRING_END_WITH_H_
#include <abel/strings/string_view.h>
#include <string>

namespace abel {

 /**
  * @brief Checks if the given suffix string is located at the end of this string.
  * @param text
  * @param suffix
  * @return
  */
ABEL_FORCE_INLINE bool ends_with(abel::string_view text, abel::string_view suffix) {
    return suffix.empty() ||
        (text.size() >= suffix.size() &&
            memcmp(text.data() + (text.size() - suffix.size()), suffix.data(),
                   suffix.size()) == 0);
}


 /**
  * @brief Checks if the given suffix string is located at the end of this
  * string. Compares the characters case-insensitively.
  * @param text
  * @param suffix
  * @return
  */
bool ends_with_case(abel::string_view text, abel::string_view suffix);

}
#endif //ABEL_STRING_END_WITH_H_
