//

#ifndef ABEL_FLAGS_INTERNAL_PATH_UTIL_H_
#define ABEL_FLAGS_INTERNAL_PATH_UTIL_H_

#include <abel/asl/string_view.h>

namespace abel {

    namespace flags_internal {

// A portable interface that returns the basename of the filename passed as an
// argument. It is similar to basename(3)
// <https://linux.die.net/man/3/basename>.
// For example:
//     flags_internal::base_name("a/b/prog/file.cc")
// returns "file.cc"
//     flags_internal::base_name("file.cc")
// returns "file.cc"
        ABEL_FORCE_INLINE abel::string_view base_name(abel::string_view filename) {
            auto last_slash_pos = filename.find_last_of("/\\");

            return last_slash_pos == abel::string_view::npos
                   ? filename
                   : filename.substr(last_slash_pos + 1);
        }

// A portable interface that returns the directory name of the filename
// passed as an argument, including the trailing slash.
// Returns the empty string if a slash is not found in the input file name.
// For example:
//      flags_internal::package("a/b/prog/file.cc")
// returns "a/b/prog/"
//      flags_internal::package("file.cc")
// returns ""
        ABEL_FORCE_INLINE abel::string_view package(abel::string_view filename) {
            auto last_slash_pos = filename.find_last_of("/\\");

            return last_slash_pos == abel::string_view::npos
                   ? abel::string_view()
                   : filename.substr(0, last_slash_pos + 1);
        }

    }  // namespace flags_internal

}  // namespace abel

#endif  // ABEL_FLAGS_INTERNAL_PATH_UTIL_H_
