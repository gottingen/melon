
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef MELON_FUTURE_DETAIL_PROMISE_H_
#define MELON_FUTURE_DETAIL_PROMISE_H_


namespace melon {

    template <typename Alloc, typename... Ts>
    basic_promise<Alloc, Ts...>::basic_promise(const Alloc& alloc) {
        storage_.allocate(alloc);
    }

    template <typename Alloc, typename... Ts>
    basic_promise<Alloc, Ts...>::~basic_promise() {
        if (storage_ && !value_assigned_) {
            storage_->fail(std::make_exception_ptr(unfull_filled_promise{}));
        }
    }

    template <typename Alloc, typename... Ts>
    typename basic_promise<Alloc, Ts...>::future_type
    basic_promise<Alloc, Ts...>::get_future() {
        assert(!future_created_);
        future_created_ = true;

        return future_type{storage_};
    }

    template <typename Alloc, typename... Ts>
    template <typename... Us>
    void basic_promise<Alloc, Ts...>::finish(Us&&... f) {
        assert(storage_ && !value_assigned_);

        storage_->finish(std::make_tuple(std::forward<Us>(f)...));
        value_assigned_ = true;

        if (future_created_) storage_.reset();
    }

    template <typename Alloc, typename... Ts>
    template <typename... Us>
    void basic_promise<Alloc, Ts...>::set_value(Us&&... vals) {
        assert(storage_ && !value_assigned_);

        storage_->full_fill(full_fill_type(std::forward<Us>(vals)...));
        value_assigned_ = true;
        if (future_created_) storage_.reset();
    }

    template <typename Alloc, typename... Ts>
    void basic_promise<Alloc, Ts...>::set_exception(fail_type e) {
        assert(storage_ && !value_assigned_);

        storage_->fail(std::move(e));
        value_assigned_ = true;

        if (future_created_) storage_.reset();
    }

    template <typename Alloc, typename... Ts>
    basic_promise<Alloc, Ts...>::operator bool() const {
        return storage_;
    }
}  // namespace melon
#endif  // MELON_FUTURE_DETAIL_PROMISE_H_
