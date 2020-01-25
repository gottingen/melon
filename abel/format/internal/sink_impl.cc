//
// Created by liyinbin on 2020/1/25.
//

#include <abel/format/internal/sink_impl.h>

namespace abel {
namespace format_internal {

bool format_sink_impl::PutPaddedString(string_view v, int w, int p, bool l) {
    size_t space_remaining = 0;
    if (w >= 0) space_remaining = w;
    size_t n = v.size();
    if (p >= 0) n = std::min(n, static_cast<size_t>(p));
    string_view shown(v.data(), n);
    space_remaining = Excess(shown.size(), space_remaining);
    if (!l) Append(space_remaining, ' ');
    Append(shown);
    if (l) Append(space_remaining, ' ');
    return true;
}

} //namespace format_internal
} //namespace abel