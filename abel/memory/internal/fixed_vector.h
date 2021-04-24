//
// Created by liyinbin on 2021/4/3.
//

#ifndef ABEL_MEMORY_FIXED_VECTOR_H_
#define ABEL_MEMORY_FIXED_VECTOR_H_

#include <memory>
#include <utility>

#include "abel/log/logging.h"

namespace abel {

    namespace memory_internal {

        class fixed_vector {
        public:
            using mske_ptr = std::pair<void *, void (*)(void *)>;

            fixed_vector() = default;

            explicit fixed_vector(std::size_t size)
                    : objects_(std::make_unique<mske_ptr[]>(size)),
                      current_(objects_.get()),
                      end_(objects_.get() + size) {}

            // We **hope** after destruction, calls to `full()` would return `true`.
            // Frankly it's U.B. anyway, we only need this behavior when thread is
            // leaving, so as to deal with thread-local destruction order issues.
            ~fixed_vector() {
                while (!empty()) {
                    auto&&[p, deleter] = *pop_back();
                    deleter(p);
                }
                current_ = end_ = nullptr;
            }

            bool empty() const noexcept { return current_ == objects_.get(); }

            std::size_t size() const noexcept { return current_ - objects_.get(); }

            bool full() const noexcept { return current_ == end_; }

            template<class... Ts>
            void emplace_back(Ts &&... args) noexcept {
                DCHECK_LT(current_, end_);
                DCHECK_GE(current_, objects_.get());
                *current_++ = {std::forward<Ts>(args)...};
            }

            mske_ptr *pop_back() noexcept {
                DCHECK_LE(current_, end_);
                DCHECK_GT(current_, objects_.get());
                return --current_;
            }

        private:
            std::unique_ptr<mske_ptr[]> objects_;
            mske_ptr *current_;
            mske_ptr *end_;
        };

    }  // namespace memory_internal
}  // namespace abel

#endif  // ABEL_MEMORY_FIXED_VECTOR_H_
