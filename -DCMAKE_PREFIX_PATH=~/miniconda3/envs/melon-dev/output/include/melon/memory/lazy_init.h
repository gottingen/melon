
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef MELON_MEMORY_LAZY_INIT_H_
#define MELON_MEMORY_LAZY_INIT_H_


#include <optional>
#include <utility>

#include "melon/log/logging.h"

namespace melon {

    template<class T>
    class lazy_init {
    public:
        template<class... Args>
        void init(Args &&... args) {
            value_.emplace(std::forward<Args>(args)...);
        }

        void destroy() { value_ = std::nullopt; }

        T *operator->() {
            MELON_DCHECK(value_);
            return &*value_;
        }

        const T *operator->() const {
            MELON_DCHECK(value_);
            return &*value_;
        }

        T &operator*() {
            MELON_DCHECK(value_);
            return *value_;
        }

        const T &operator*() const {
            MELON_DCHECK(value_);
            return *value_;
        }

        explicit operator bool() const { return !!value_; }

    private:
        std::optional<T> value_;
    };

}  // namespace melon

#endif  // MELON_MEMORY_LAZY_INIT_H_
