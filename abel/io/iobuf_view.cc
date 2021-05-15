//
// Created by liyinbin on 2021/4/19.
//


#include "iobuf_view.h"

#include <algorithm>
#include <utility>

namespace abel {

    iobuf_view::iobuf_view(){}
            //: iobuf_view(/*early_init_constant<iobuf>()*/) {}

    iobuf_view::iobuf_view(
            const iobuf &buffer) {
        byte_size_ = buffer.byte_size();
        auto offset = 0;
        for (auto iter = buffer.begin(); iter != buffer.end(); ++iter) {
            offsets_.emplace_back(offset, iter);
            offset += iter->size();
        }
        offsets_.emplace_back(offset, buffer.end());
        DCHECK_EQ(static_cast<size_t >(offset), buffer.byte_size());
    }

    std::pair<std::size_t, iobuf::const_iterator>
    iobuf_view::find_segment_must_succeed(
            std::size_t offset) const noexcept {
        DCHECK(offset <= size(),
                   "Invalid offset [{}]. The buffer is only {} bytes long.",
                   offset, size());
        auto iter = std::upper_bound(
                offsets_.begin(), offsets_.end(), offset,
                [](auto value, auto &&entry) { return value < entry.first; });
        DCHECK(iter != offsets_.begin());
        --iter;
        DCHECK_LE(iter->first, offset);
        if (iter != offsets_.end() - 1) {
            DCHECK_LT(offset, iter->first + iter->second->size());
        }
        return *iter;
    }

}  // namespace abel
