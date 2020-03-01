//
// Created by liyinbin on 2018/10/24.
//

#ifndef ABEL_STRINGS_COMPARE_H_
#define ABEL_STRINGS_COMPARE_H_

#include <abel/strings/string_view.h>

namespace abel {
/**
 * @brief returns +1/0/-1 like strcmp(a, b) but without regard for letter case
 * @param a
 * @param b
 * @return
 */
    int compare_case(abel::string_view a, abel::string_view b);

    ABEL_FORCE_INLINE bool equal_case(abel::string_view a, abel::string_view b) {
        return compare_case(a, b) == 0;
    }

} //namespace abel

#endif //ABEL_STRINGS_COMPARE_H_
