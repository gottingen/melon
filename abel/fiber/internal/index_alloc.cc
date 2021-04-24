//
// Created by liyinbin on 2021/4/17.
//

#include "abel/fiber/internal/index_alloc.h"

namespace abel {

        std::size_t index_alloc::next() {
            std::scoped_lock _(lock_);
            if (!recycled_.empty()) {
                auto rc = recycled_.back();
                recycled_.pop_back();
                return rc;
            }
            return current_++;
        }

        void index_alloc::free(std::size_t index) {
            std::scoped_lock _(lock_);
            recycled_.push_back(index);
        }
}  // namespace abel
