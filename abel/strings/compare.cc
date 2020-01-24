//
// Created by liyinbin on 2020/1/24.
//

#include <abel/strings/compare.h>
#include <abel/strings/ascii.h>
namespace abel {

int compare_case (abel::string_view a, abel::string_view b){
    abel::string_view::const_iterator ai = a.begin();
    abel::string_view::const_iterator bi = b.begin();

    while (ai != a.end() && bi != b.end()) {
        int ca = ascii::to_lower(*ai++);
        int cb = ascii::to_lower(*bi++);

        if (ca == cb) {
            continue;
        }
        if (ca < cb) {
            return -1;
        } else {
            return +1;
        }
    }

    if (ai == a.end() && bi != b.end()) {
        return +1;
    } else if (ai != a.end() && bi == b.end()) {
        return -1;
    } else {
        return 0;
    }
}

} //namespace abel