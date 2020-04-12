//

#include <abel/strings/str_split.h>

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <limits>
#include <memory>

#include <abel/log/abel_logging.h>
#include <abel/asl/ascii.h>

namespace abel {


    namespace {

        // This generic_find() template function encapsulates the finding algorithm
        // shared between the by_string and by_any_char delimiters. The FindPolicy
        // template parameter allows each delimiter to customize the actual find
        // function to use and the length of the found delimiter. For example, the
        // Literal delimiter will ultimately use abel::string_view::find(), and the
        // AnyOf delimiter will use abel::string_view::find_first_of().
        template<typename FindPolicy>
        abel::string_view generic_find(abel::string_view text,
                                      abel::string_view delimiter, size_t pos,
                                      FindPolicy find_policy) {
            if (delimiter.empty() && text.length() > 0) {
                // Special case for empty std::string delimiters: always return a zero-length
                // abel::string_view referring to the item at position 1 past pos.
                return abel::string_view(text.data() + pos + 1, 0);
            }
            size_t found_pos = abel::string_view::npos;
            abel::string_view found(text.data() + text.size(),
                                    0);  // By default, not found
            found_pos = find_policy.find(text, delimiter, pos);
            if (found_pos != abel::string_view::npos) {
                found = abel::string_view(text.data() + found_pos,
                                          find_policy.Length(delimiter));
            }
            return found;
        }

// Finds using abel::string_view::find(), therefore the length of the found
// delimiter is delimiter.length().
        struct literal_policy {
            size_t find(abel::string_view text, abel::string_view delimiter, size_t pos) {
                return text.find(delimiter, pos);
            }

            size_t Length(abel::string_view delimiter) { return delimiter.length(); }
        };

// Finds using abel::string_view::find_first_of(), therefore the length of the
// found delimiter is 1.
        struct any_of_policy {
            size_t find(abel::string_view text, abel::string_view delimiter, size_t pos) {
                return text.find_first_of(delimiter, pos);
            }

            size_t Length(abel::string_view /* delimiter */) { return 1; }
        };

    }  // namespace

//
// by_string
//

    by_string::by_string(abel::string_view sp) : _delimiter(sp) {}

    abel::string_view by_string::find(abel::string_view text, size_t pos) const {
        if (_delimiter.length() == 1) {
            // Much faster to call find on a single character than on an
            // abel::string_view.
            size_t found_pos = text.find(_delimiter[0], pos);
            if (found_pos == abel::string_view::npos)
                return abel::string_view(text.data() + text.size(), 0);
            return text.substr(found_pos, 1);
        }
        return generic_find(text, _delimiter, pos, literal_policy());
    }

//
// by_char
//

    abel::string_view by_char::find(abel::string_view text, size_t pos) const {
        size_t found_pos = text.find(c_, pos);
        if (found_pos == abel::string_view::npos)
            return abel::string_view(text.data() + text.size(), 0);
        return text.substr(found_pos, 1);
    }

//
// by_any_char
//

    by_any_char::by_any_char(abel::string_view sp) : _delimiters(sp) {}

    abel::string_view by_any_char::find(abel::string_view text, size_t pos) const {
        return generic_find(text, _delimiters, pos, any_of_policy());
    }

//
//  by_length
//
    by_length::by_length(ptrdiff_t length) : _length(length) {
        ABEL_RAW_CHECK(length > 0, "");
    }

    abel::string_view by_length::find(abel::string_view text,
                                      size_t pos) const {
        pos = std::min(pos, text.size());  // truncate `pos`
        abel::string_view substr = text.substr(pos);
        // If the std::string is shorter than the chunk size we say we
        // "can't find the delimiter" so this will be the last chunk.
        if (substr.length() <= static_cast<size_t>(_length))
            return abel::string_view(text.data() + text.size(), 0);

        return abel::string_view(substr.data() + _length, 0);
    }


}  // namespace abel
