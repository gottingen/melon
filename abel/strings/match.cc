//

#include <abel/strings/match.h>

#include <abel/strings/internal/memutil.h>

namespace abel {


bool EqualsIgnoreCase(abel::string_view piece1, abel::string_view piece2) {
  return (piece1.size() == piece2.size() &&
          0 == abel::strings_internal::memcasecmp(piece1.data(), piece2.data(),
                                                  piece1.size()));
  // memcasecmp uses abel::ascii_tolower().
}

bool starts_with_case(abel::string_view text, abel::string_view prefix) {
  return (text.size() >= prefix.size()) &&
         EqualsIgnoreCase(text.substr(0, prefix.size()), prefix);
}

bool ends_with_case(abel::string_view text, abel::string_view suffix) {
  return (text.size() >= suffix.size()) &&
         EqualsIgnoreCase(text.substr(text.size() - suffix.size()), suffix);
}


}  // namespace abel
