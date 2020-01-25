#include <abel/format/internal/format_flags.h>

namespace abel {
namespace format_internal {

std::string format_flags::to_string() const {
    std::string s;
    s.append(left     ? "-" : "");
    s.append(show_pos ? "+" : "");
    s.append(sign_col ? " " : "");
    s.append(alt      ? "#" : "");
    s.append(zero     ? "0" : "");
    return s;
}

}
}